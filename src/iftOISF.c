#include "iftOISF.h"
//----------------------------------------------------------------------------//
// PRIVATE FUNCTIONS
//----------------------------------------------------------------------------//
/**
* BRIEF
*    Adds a new band for the object saliency map values into the MImage
*
* DESCRIPTION 
*    This function creates a new MImage by copying the LAB and saliency values
*    in the following order: [0..2] - CIELAB; and [3] - Normalized Saliency.
*    The idea of normalizing the saliency values is to avoid feature domination
*    during the OISF execution.
*
* PARAMETERS
*    mimg     - Multiband image
*    objsm    - Object saliency map
*
* RETURN
*    Multiband image composed of CIELAB and normalized saliency bands
*/
iftMImage *_iftExtendMImageByObjSalMap
(iftMImage *mimg, iftImage* objsm )
{
  int p,b, min_sm_val, max_sm_val;
  float max_lab_val;
  iftMImage *emimg;

  emimg = iftCreateMImage(mimg->xsize, mimg->ysize, mimg->zsize, mimg->m+1);
  max_lab_val = iftMMaximumValue(mimg, -1);
  iftMinMaxValues(objsm, &min_sm_val, &max_sm_val);
  
  for (p = 0; p < mimg->n; p++)  {
    
    for(b = 0; b < mimg->m; b++ )  emimg->val[p][b] = mimg->val[p][b];

    // Normalize for avoiding feature domination
    emimg->val[p][mimg->m] = max_lab_val * ((objsm->val[p] - min_sm_val)/((float)(max_sm_val - min_sm_val)));
  }

  return emimg;
}

//----------------------------------------------------------------------------//
// PUBLIC FUNCTIONS
//----------------------------------------------------------------------------//
iftIGraph *iftInitOISFIGraph
(iftImage *img, iftImage *mask, iftImage *objsm)
{
  iftMImage *mimg, *obj_mimg;
  iftAdjRel *A;
  iftIGraph *igraph;

  A = iftCircular(1.0);

  if (iftIsColorImage(img)) mimg   = iftImageToMImage(img,LABNorm_CSPACE);
  else mimg   = iftImageToMImage(img,GRAY_CSPACE);

  obj_mimg = _iftExtendMImageByObjSalMap(mimg, objsm);

  igraph = iftImplicitIGraph(obj_mimg, mask, A);

  //Free
  iftDestroyMImage(&mimg);
  iftDestroyMImage(&obj_mimg);
  iftDestroyAdjRel(&A);

  return igraph;
}

void iftOISF
(iftIGraph *igraph, iftImage *seeds, double alpha, double beta, double gamma, int iters)
{
  double tmp;
  int r, s, t, i, p, q, it, nseeds;
  int *seed, *center;
  float max_objsm_val;
  double color_dist, geo_dist, obj_dist;
  iftVoxel u, v;
  iftDHeap *Q;
  double *pvalue;
  iftSet *S, *new_seeds, *frontier_nodes, *trees_rm;

  nseeds = 0;
  max_objsm_val = iftIGraphMaximumFeatureValue(igraph, igraph->nfeats-1);
  
  S = NULL;
  new_seeds = NULL;
  frontier_nodes = NULL;
  trees_rm = NULL;

  pvalue = iftAllocDoubleArray(igraph->nnodes);
  Q = iftCreateDHeap(igraph->nnodes, pvalue);

  for (s=0; s < igraph->nnodes; s++) 
  {
      p               = igraph->node[s].voxel;
      pvalue[s]       = IFT_INFINITY_DBL;
      igraph->pvalue[p] = IFT_INFINITY_DBL;
      igraph->pred[p] = IFT_NIL;
      
      if (seeds->val[p]!=0)
      {
          iftInsertSet(&new_seeds,s);
          nseeds++;
      }
  }

  seed = iftAllocIntArray(nseeds);
  S = new_seeds; 
  i = 0;

  while (S != NULL) 
  {
      seed[i] = S->elem;
      p       = igraph->node[seed[i]].voxel;
      igraph->label[p] = i+1;
      i++; 
      S = S->next;
  }
  
  for (it=0; it < iters; it++) 
  {
    if (trees_rm != NULL)
    { 
      frontier_nodes = iftIGraphTreeRemoval(igraph, &trees_rm, pvalue, IFT_INFINITY_DBL);
    }

    while (new_seeds != NULL) 
    {
      s = iftRemoveSet(&new_seeds);
      p = igraph->node[s].voxel;  

      if (igraph->label[p] > 0)
      { 
        pvalue[s] = 0;
        igraph->pvalue[p] = 0;
        igraph->root[p] = p;
        igraph->pred[p] = IFT_NIL;
        iftInsertDHeap(Q,s);
      }
    }

    while (frontier_nodes != NULL) 
    {
      s = iftRemoveSet(&frontier_nodes);
      
      if (Q->color[s] == IFT_WHITE) iftInsertDHeap(Q,s);
    } 

    while (!iftEmptyDHeap(Q)) 
    {
      s = iftRemoveDHeap(Q);
      p = igraph->node[s].voxel;
      r = igraph->root[p];
      igraph->pvalue[p] = pvalue[s];
      u = iftGetVoxelCoord(igraph->index,p);

      for (i=1; i < igraph->A->n; i++) 
      {
        v = iftGetAdjacentVoxel(igraph->A,u,i);
        if (iftValidVoxel(igraph->index,v))
        {
          q   = iftGetVoxelIndex(igraph->index,v);
          t   = igraph->index->val[q];
          if ((t != IFT_NIL) && (Q->color[t] != IFT_BLACK))
          {
            tmp = 0.0;
            
            color_dist = (double)iftFeatDistance(igraph->feat[r], igraph->feat[q], igraph->nfeats-1);
            geo_dist = (double)iftVoxelDistance(u,v);
            obj_dist = (double)(abs((igraph->feat[r][igraph->nfeats-1] - igraph->feat[q][igraph->nfeats-1]))/max_objsm_val              );          
            
            tmp = pow( alpha*color_dist*pow(gamma, obj_dist) +gamma*obj_dist, beta);
            tmp += geo_dist;
            tmp += pvalue[s];

            if (tmp < pvalue[t])
            {
              pvalue[t]            = tmp;

              igraph->root[q]      = igraph->root[p];
              igraph->label[q]     = igraph->label[p];
              igraph->pred[q]      = p;

              if (Q->color[t] == IFT_GRAY) iftGoUpDHeap(Q, Q->pos[t]);
              else iftInsertDHeap(Q,t);
            } 
            else 
            {
              if (igraph->pred[q] == p)
              {
                if (tmp > pvalue[t]) iftIGraphSubTreeRemoval(igraph,t,pvalue,IFT_INFINITY_DBL,Q);
                else 
                {
                  if ((igraph->label[q] != igraph->label[p])&&(igraph->label[q]!=0))
                  {
                    iftIGraphSubTreeRemoval(igraph,t,pvalue,IFT_INFINITY_DBL,Q);
                  }
                }
              }
            }
          } 
        }
      }
    } 

    iftResetDHeap(Q);



    if( iters > 1 ) {
      center = NULL;
      center = iftIGraphSuperpixelCenters(igraph, seed, nseeds);

      iftIGraphEvalAndAssignNewSeeds(igraph, center, seed, nseeds, &trees_rm, &new_seeds);
    }
  }

  // Free
  iftDestroySet(&S);
  iftDestroySet(&new_seeds);
  iftDestroySet(&frontier_nodes);
  iftDestroySet(&trees_rm);
  iftDestroyDHeap(&Q);
  iftFree(pvalue);
  iftFree(seed);
}
