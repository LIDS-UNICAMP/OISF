#include "iftOSMOX.h"

//----------------------------------------------------------------------------//
// PRIVATE FUNCTIONS
//----------------------------------------------------------------------------//
/**
* BRIEF
*    Selects the quantity of seeds based on their accumulated value
*
* DESCRIPTION 
*    This algorithm selects the pixels with highest accumulated (based on its
*    adjacency) as seeds. In order to assure a fair distribution, the <stddev>
*    controls the proximity amongst them. Finally, the user can provide a mask 
*    image defining the ROI where all seeds can be placed. 
*
* PARAMETERS
*    objsm     - Object saliency map
*    mask      - ROI image (can be set to NULL)
*    num_seeds - Number of seeds to be sampled (x > 0)
*    stddev    - Seed proximity factor (x > 0)
*
* RETURN
*    Set of seeds' indexes
*/

iftSet *_iftObjSalMapSamplByValueWithAreaSum
(iftImage *objsm, iftImage *mask, int num_seeds, float stddev)
{
  // 1. Input Validation -------------------------------------------------------
  if(objsm->n < num_seeds || num_seeds < 0) 
    iftError("Invalid number of seeds!", 
             "_iftObjSalMapSamplByValueWithAreaSum");
  if(stddev <= 0.0) 
    iftError("Invalid proximity value!", 
             "_iftObjSalMapSamplByValueWithAreaSum");

  if(mask != NULL) 
    iftVerifyImageDomains(objsm, mask, "_iftObjSalMapSamplByValueWithAreaSum");

  // 2. Create Aux Vars --------------------------------------------------------
  int total_area;
  iftImage *mask_copy;

  if( mask == NULL ) 
    mask_copy = iftSelectImageDomain(objsm->xsize, objsm->ysize, objsm->zsize);
  else mask_copy = mask;
  
  total_area = 0;

  // Compute the total area avaliable for sampling
  #ifdef IFT_PARALLEL
  #pragma omp parallel for reduction(+:total_area)
  #endif
  for(int p = 0; p < objsm->n; p++)
  {
    if(mask_copy->val[p] != 0) total_area++;
  }

  // 3. Create Gaussian Penalization -------------------------------------------
  int patch_width;
  float stdev;
  iftAdjRel *A;
  iftKernel *gaussian;

  // Estimate the gaussian influence zone
  patch_width = iftRound(sqrtf(total_area/(float)(num_seeds)));
  stdev = patch_width/stddev; // Recommended: 6.0
 
  A = iftCircular(patch_width); 
  gaussian = iftCreateKernel(A);

  #ifdef IFT_PARALLEL
  #pragma omp parallel for
  #endif
  for(int i = 0; i < A->n; i++) 
  {
    float dist;

    dist = A->dx[i]*A->dx[i] + A->dy[i]*A->dy[i];
    gaussian->weight[i] = exp(-dist/(2*stdev*stdev));
  }

  iftDestroyAdjRel(&A);

  // 4. Priority Computation ---------------------------------------------------
  double *pixel_val;
  iftAdjRel *B;
  iftDHeap *heap;

  B = iftCircular(sqrtf(patch_width)); // For speed-up purposes

  pixel_val = (double *)calloc(objsm->n, sizeof(double));
  heap = iftCreateDHeap(objsm->n, pixel_val);

  iftSetRemovalPolicyDHeap(heap, MAXVALUE);
  #ifdef IFT_PARALLEL
  #pragma omp parallel for
  #endif
  for( int p = 0; p < objsm->n; p++ ) {
    if(mask_copy->val[p] != 0) {
      iftVoxel p_voxel;
      
      p_voxel = iftGetVoxelCoord(objsm, p);
      
      pixel_val[p] = objsm->val[p];

      // For each adjacent in the neighborhood
      for(int i = 1; i < B->n; i++)
      {
        iftVoxel q_voxel;
        
        q_voxel = iftGetAdjacentVoxel(B, p_voxel, i);
        
        if(iftValidVoxel(objsm, q_voxel))
        {
          int q;
          
          q = iftGetVoxelIndex(objsm, q_voxel);
          
          pixel_val[p] += objsm->val[q];  
        }
      }
    }
    else pixel_val[p] = IFT_NIL;
  }
  
  for( int p = 0; p < objsm->n; p++ ) 
    if(pixel_val[p] != IFT_NIL) iftInsertDHeap(heap, p);
  
  iftDestroyAdjRel(&B);
  if(mask == NULL) iftDestroyImage(&mask_copy);

  // 5. Seed Sampling ----------------------------------------------------------
  int seed_count;
  iftSet *seed;

  seed = NULL;
  seed_count = 0;

  while( seed_count < num_seeds && !iftEmptyDHeap(heap) ) 
  {
    int p;
    iftVoxel voxel_p;

    p = iftRemoveDHeap(heap);
    voxel_p = iftGetVoxelCoord(objsm, p);

    iftInsertSet(&seed, p);

    // Mark as removed
    pixel_val[p] = IFT_NIL;

    // For every adjacent voxel in the influence zone
    for(int i = 1; i < gaussian->A->n; i++) {
      iftVoxel voxel_q;

      voxel_q = iftGetAdjacentVoxel(gaussian->A, voxel_p, i);

      if(iftValidVoxel(objsm, voxel_q)) {
        int q;

        q = iftGetVoxelIndex(objsm, voxel_q);
        
        // If it was not removed (yet)  
        if( pixel_val[q] != IFT_NIL ) {
          // Penalize
          pixel_val[q] = (1.0 - gaussian->weight[i]) * pixel_val[q];

          iftGoDownDHeap(heap, heap->pos[q]);
        }
      }
    }

    seed_count++;
  }

  free(pixel_val);
  iftDestroyKernel(&gaussian);
  iftDestroyDHeap(&heap);

  return (seed);
}

//----------------------------------------------------------------------------//
// PUBLIC FUNCTIONS
//----------------------------------------------------------------------------//
iftImage *iftOSMOX
(iftImage* objsm, iftImage *mask, int num_seeds, float obj_perc, float stddev)
{
  // 1. Input Validation -------------------------------------------------------
  if(objsm->n < num_seeds || num_seeds < 0) 
    iftError("Invalid number of seeds!", "iftOSMOX"); 
  if(obj_perc < 0.0 || obj_perc > 1.0) 
    iftError("Invalid object percentage!", "iftOSMOX");
  if(stddev <= 0.0) 
    iftError("Invalid standard deviation value!", "iftOSMOX");
  if(mask != NULL) iftVerifyImageDomains(objsm, mask, "iftOSMOX");

  // 2. Create Aux Vars --------------------------------------------------------
  int obj_seeds, bkg_seeds, max_val, min_val;
  iftImage *mask_copy;
  
  // Establish the number of seeds
  iftMinMaxValues(objsm, &min_val, &max_val);

  if(max_val == min_val) // Only one value (e.g., all black/white)
  {
    obj_seeds = num_seeds;
    bkg_seeds = 0;
  }
  else
  {
    obj_seeds = iftRound(num_seeds * obj_perc);
    bkg_seeds = num_seeds - obj_seeds;
  }

  if( mask == NULL ) 
    mask_copy = iftSelectImageDomain(objsm->xsize, objsm->ysize, objsm->zsize);
  else mask_copy = iftCopyImage(mask);

  // 3. Seed sampling ----------------------------------------------------------
  iftSet *obj_set, *bkg_set, *s;
  iftImage *seed_img, *invsm;

  seed_img = iftCreateImage(objsm->xsize, objsm->ysize, objsm->zsize);

  obj_set = _iftObjSalMapSamplByValueWithAreaSum(objsm, mask_copy, obj_seeds, stddev);

  s = obj_set;
  while( s != NULL ) {
    mask_copy->val[s->elem] = 0; // Avoid resampling in the same location
    seed_img->val[s->elem] = 1;
    s = s->next;
  }
  iftDestroySet(&obj_set);

  // Background importance is the complement of the objects'
  invsm = iftComplement(objsm);
  
  bkg_set = _iftObjSalMapSamplByValueWithAreaSum(invsm, mask_copy, bkg_seeds, stddev);
  iftDestroyImage(&mask_copy);
  iftDestroyImage(&invsm);

  s = bkg_set;
  while( s != NULL ) {
    seed_img->val[s->elem] = 1;
    s = s->next;
  }
  iftDestroySet(&bkg_set);  
  
  return (seed_img);
}

