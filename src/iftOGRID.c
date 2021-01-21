#include "iftOGRID.h"

//----------------------------------------------------------------------------//
// PRIVATE FUNCTIONS
//----------------------------------------------------------------------------//
/**
* BRIEF
*    Samples equidistant seeds within each object defined by the label map.
*
* DESCRIPTION 
*    This algorithm assigns the quantity of seeds proportionally to the objects'
*    area, such that smaller objects (and probable noises) will contain few or
*    no seeds. Finally, the user can provide a mask image defining the ROI 
*    where all seeds can be placed. 
*
* PARAMETERS
*    label     - Label map
*    mask      - ROI image (can be set to NULL)
*    nSeeds    - Number of seeds to be sampled (x > 0)
*
* RETURN
*    Set of seeds' indexes
*/
iftSet *_iftMultiLabelGridSamplingOnMaskByArea
(const iftImage *label, iftImage *mask, int nSeeds )
{
  // 1. Input Validation -------------------------------------------------------
  if(label->n < nSeeds || nSeeds < 0) 
    iftError("Invalid number of seeds!", 
             "_iftMultiLabelGridSamplingOnMaskByArea");
  if(mask != NULL) 
    iftVerifyImageDomains(label, mask, "_iftMultiLabelGridSamplingOnMaskByArea");

  // 2. Create Aux Vars --------------------------------------------------------
  int totalArea, numObj, max_num_seeds;
  iftImage *newLabels;
  iftAdjRel *A;

  totalArea = 0;
  A = iftCircular(1.45);

  newLabels = iftFastLabelComp(label, A);
  iftDestroyAdjRel(&A);
  numObj = iftMaximumValue(newLabels);
  
  #ifdef IFT_PARALLEL
  #pragma omp parallel for reduction(+:totalArea)
  #endif
  for(int p = 0; p < newLabels->n; p++ ) if(newLabels->val[p] > 0) totalArea++;
  
  max_num_seeds = iftMin(totalArea, nSeeds);
  
  // 3. Sampling --------------------------------------------------------------
  iftSet *seeds;

  seeds = NULL;

  #ifdef IFT_PARALLEL
  #pragma omp parallel for
  #endif
  for( int i = 1; i <= numObj; i++ ) 
  {  
    // 3.1. Assign the number of seeds ----------------------------------------
    int objArea, amount_seeds;
    float objPerc;
    iftImage* objMask;

    objArea = 0;

    objMask = iftThreshold(newLabels, i, i, 1);
    for(int p = 0; p < objMask->n; p++ ) if(objMask->val[p] > 0) objArea++;

    objPerc = objArea / (float)totalArea; // How big is this object?
    amount_seeds = iftRound(max_num_seeds * objPerc);

    // 3.2. Sample seeds within the object ------------------------------------
    if( amount_seeds > 0 ) {
      float radius;
      iftIntArray *sampled;
      
      radius = iftEstimateGridOnMaskSamplingRadius(objMask, -1, amount_seeds);
      sampled = iftGridSamplingOnMask(objMask, radius, -1, 0);

      for(int j = 0; j < sampled->n; j++) 
      {
        if(mask->val[sampled->val[j]] != 0) 
        {
          #ifdef IFT_PARALLEL
          #pragma omp critical
          #endif
          iftInsertSet(&seeds, sampled->val[j]);
        } 
      }

      iftDestroyIntArray(&sampled);
    }

    iftDestroyImage(&objMask);
  }
  
  iftDestroyImage(&newLabels);
  
  return seeds;
}

//----------------------------------------------------------------------------//
// PUBLIC FUNCTIONS
//----------------------------------------------------------------------------//
iftImage *iftOGRID
(iftImage* objsm, iftImage *mask, int num_seeds, float obj_perc, float thr)
{
  // 1. Input Validation -------------------------------------------------------
  if(objsm->n < num_seeds || num_seeds < 0) 
    iftError("Invalid number of seeds!", "iftOGRID");
  if(obj_perc < 0 || obj_perc > 1) 
    iftError("Invalid object percentage!", "iftOGRID");
  if(thr < 0 || thr > 1) 
    iftError("Invalid threshold value!", "iftOGRID");
  if(mask != NULL) 
    iftVerifyImageDomains(objsm, mask, "iftOGRID");

  // 2. Create Aux Vars --------------------------------------------------------
  int min, max;
  iftImage *seed_img;

  iftMinMaxValues(objsm, &min, &max);
  seed_img = iftCreateImage(objsm->xsize, objsm->ysize, objsm->zsize);
  
  // 3. Seed sampling ----------------------------------------------------------
  int nseeds;
  iftSet* seeds, *S;
  iftImage *bin;

  seeds = NULL; S = NULL;

  // Background importance is the complement of the objects'
  bin = iftThreshold(objsm, min, thr*max, 1);
  nseeds = iftMax( iftRound(num_seeds * (1 - obj_perc)) , 1 ); // Always > 0
  seeds = _iftMultiLabelGridSamplingOnMaskByArea(bin, mask, nseeds);
  iftDestroyImage(&bin);

  S = seeds;  
  while (S != NULL) { seed_img->val[S->elem] = 1; S = S->next;}
  iftDestroySet(&seeds);

  bin = iftThreshold(objsm, thr*max, max, 1);
      
  nseeds = iftMax( iftRound(num_seeds * obj_perc) , 1 ); // Always > 0
  seeds = _iftMultiLabelGridSamplingOnMaskByArea(bin, mask, nseeds);
  iftDestroyImage(&bin);

  S = seeds;  
  while (S != NULL) { seed_img->val[S->elem] = 1; S = S->next;}
  iftDestroySet(&seeds);
  
  return seed_img;
}