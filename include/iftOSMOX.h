#ifndef _IFT_OSMOX_H
#define _IFT_OSMOX_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ift.h"

/**
* BRIEF
*    Samples the desired number of seeds using the OSMOX algorithm
*
* DESCRIPTION 
*     This algorithm selects seeds with respect to their accumulated saliency 
*     value, assuring the exact the total quantity desired by the user. The 
*     user may control the percentage of object seeds and their proximity to 
*     each other. Finally, the user can provide a mask image defining the ROI 
*     where all seeds can be placed. 
*
* PARAMETERS
*     objsm     - Object saliency map
*     mask      - ROI image (can be set to NULL)
*     num_seeds - Number of seeds to be sampled (x > 0)
*     obj_perc  - Percentage of object seeds (x in [0,1])
*     stddev    - Seed proximity factor (x > 0)
*
* RETURN
*     Image whose non-black values (i.e., non-zero luminosity) indicate a seed 
*     position
*/
iftImage *iftOSMOX
(iftImage* objsm, iftImage *mask, int num_seeds, float obj_perc, float stddev);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_IFT_OSMOX_H