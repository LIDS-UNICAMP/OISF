#ifndef _IFT_OISF_H_
#define _IFT_OISF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ift.h"

/**
* BRIEF
*    Creates an image graph especially designed for the OISF algorithm
*
* DESCRIPTION 
*     This method creates a graph derived from the image for a proper 
*     execution of the OISF algorithm. It converts the image to CIELAB 
*     (D65 whitepoint reference) and normalizes the object saliency
*     map features in order to avoid feature domination. The user may 
*     provide a mask image defining the ROI delimiting the segmentation 
*     limits.
*
* PARAMETERS
*     img     - Original image
*     objsm   - Object saliency map
*     mask    - ROI image (can be set to NULL)
*
* RETURN
*     Image graph with normalized features
*/
iftIGraph *iftInitOISFIGraph
(iftImage *img, iftImage *mask, iftImage *objsm);

/**
* BRIEF
*    Segments inplace the image graph using the OISF algorithm
*
* DESCRIPTION 
*     This function segments the nodes defined in the image graph
*     using the OISF algorithm (see iftInitOISFIGraph). By considering
*     the prior object location knowledge, OISF permits the user to 
*     control the regularity of the superpixels (alpha), their adherence
*     to the borders defined in the color space (beta) and in the saliency
*     map space (gamma). The seeds are best located (for an effective
*     segmentation) when sampled using object-based strategies (see iftOSMOX
*     and iftOGRID). Finally, the user can control the number of iterations
*     (i.e., number of Image Foresting Transforms in the graph) in order to
*     balance speed and efficacy.
*
* PARAMETERS
*     igraph    - Original image
*     seeds     - Object saliency map
*     alpha     - Regularization factor (x > 0)
*     beta      - Boundary adherence factor (x > 0)
*     gamma     - Saliency map confidence factor (x > 0)
*     iters     - Number of iterations for segmentation (x > 0)
*/
void iftOISF
(iftIGraph *igraph, iftImage *seeds, double alpha, double beta, double gamma, int iters);

#ifdef __cplusplus
}
#endif

#endif //_IFT_OISF_H_