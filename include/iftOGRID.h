#ifndef _IFT_OSMOX_H
#define _IFT_OSMOX_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ift.h"

/**
* BRIEF
*    Samples the desired number of seeds using the OGRID algorithm
*
* DESCRIPTION 
*     This algorithm selects equally distanced seeds with respect to the
*     component's shape. Each component has a proportional quantity of seeds
*     to be distributed within, and it is subject to the threshold value. The
*     user may select such threshold, the percentage of object seeds, and may
*     provide a mask image defining the ROI where all seeds can be placed.
*
* PARAMETERS
*     objsm     - Object saliency map
*     mask      - ROI image (can be set to NULL)
*     num_seeds - Number of seeds to be sampled (x > 0)
*     obj_perc  - Percentage of object seeds (x in [0,1])
*     thr       - Threshold value (x in [0,1])
*
* RETURN
*     Image whose non-black values (i.e., non-zero luminosity) indicate a seed 
*     position
*/
iftImage *iftOGRID
(iftImage* objsm, iftImage *mask, int num_seeds, float obj_perc, float thr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_IFT_OSMOX_H