#include "ift.h"
#include "iftOGRID.h"
#include "iftOISF.h"

#define UAI_ARGS_SOURCE
#include "UAIArgs.h"

#define HELP_MSG \
    "This is a demo program for the OISF-OGRID algorithm. Usage:\n\n"\
    "  ./iftOISF_OGRID [parameters]\n"\
    "\nRequired parameters:\n"\
    "  --img=STR      Path to the input image (.png, .jpg, .pgm, .ppm)\n"\
    "  --objsm=STR    Path to the object saliency map image (.png, .jpg, .pgm)\n"\
    "  --k=INT        Desired number of superpixels (k > 1)\n"\
    "  --labels=STR   Path to the output label image (.png, .jpg, .pgm)\n"\
    "\nOptional parameters for OGRID:\n"\
    "  --perc=FLT     Object seeds' percentage (0 <= perc <= 1, default:0.9)\n"\
    "  --thr=FLT      Saliency map threshold (0 <= thr <= 1, default:0.5)\n"\
    "\nOptional parameters for OISF:\n"\
    "  --alpha=FLT    Regularity factor (alpha > 0, default:0.5)\n"\
    "  --beta=FLT     Boundary adherence factor (beta > 0, default:12.0)\n"\
    "  --gamma=FLT    Saliency map confidence factor (gamma > 0, default:2.0)\n"\
    "  --iters=INT    Number of iterations for segmentation (iters > 0, default:10)\n"\
    "\nOther optional parameters:\n"\
    "  --mask=STR     Mask for delimiting a ROI (.png, .jpg, .pgm)\n"\
    "  --ovlay=STR    Path to the output image with overlayed borders (.png, .jpg, .pgm, .ppm)\n"\
    "  --help         Prints this message\n"

int main(int argc, char const *argv[])
{
    const char *PARAM;
    // 1. Input Validation -----------------------------------------------------
    bool has_required;
    int k;
    iftImage *img, *objsm;
    
    has_required = UAIArgsExists(argc, argv, "img") &&
                   UAIArgsExists(argc, argv, "objsm") &&
                   UAIArgsExists(argc, argv, "k") &&
                   UAIArgsExists(argc, argv, "labels");
    if(!has_required || UAIArgsExists(argc, argv, "help"))
    {
        puts(HELP_MSG);
        exit(EXIT_FAILURE);
    }

    PARAM = UAIArgsGet(argc, argv, "img");
    if(PARAM == NULL) iftError("No image was given!", "main");
    img = iftReadImageByExt(PARAM);

    if(iftIs3DImage(img)) iftError("Only 2D images are permitted!", "main");

    PARAM = UAIArgsGet(argc, argv, "objsm");
    if(PARAM == NULL) iftError("No object saliency map was given!", "main");
    objsm = iftReadImageByExt(PARAM);

    iftVerifyImageDomains(img, objsm, "main");

    PARAM = UAIArgsGet(argc, argv, "k");
    if(PARAM == NULL) iftError("No superpixel quantity was given!", "main");
    k = atoi(PARAM);

    if(k <= 1) iftError("Invalid quantity of superpixels!", "main");

    PARAM = UAIArgsGet(argc, argv, "labels");
    if(PARAM == NULL) iftError("No output path was given!", "main");

    // 2. Graph Creation -------------------------------------------------------
    iftImage *mask;
    iftIGraph *graph;

    if(UAIArgsExists(argc, argv, "mask"))
    {    
        PARAM = UAIArgsGet(argc, argv, "mask");
        if(PARAM == NULL) iftError("No mask path was given!", "main");
        else mask = iftReadImageByExt(PARAM);
        
        iftVerifyImageDomains(img, mask, "main");
    }
    else mask = iftSelectImageDomain(img->xsize, img->ysize, img->zsize);

    graph = iftInitOISFIGraph(img, mask, objsm);
    
    // 3. OGRID Sampling -------------------------------------------------------
    iftImage *seed_img;
    float perc, thr;

    if(UAIArgsExists(argc, argv, "perc"))
    {
        PARAM = UAIArgsGet(argc, argv, "perc");
        if(PARAM == NULL) 
            iftError("No percentage value was given!", "main");
        else perc = atof(PARAM);
    
        if(perc < 0.0 || perc > 1.0) 
            iftError("Invalid percentage of object seeds!", "main");
    }
    else perc = 0.9;

    if(UAIArgsExists(argc, argv, "thr"))
    {
        PARAM = UAIArgsGet(argc, argv, "thr");
        if(PARAM == NULL)
            iftError("No threshold value was given!", "main");
        else thr = atof(PARAM);
        
        if(thr < 0.0 || thr > 1.0) iftError("Invalid threshold value!", "main");
    } 
    else thr = 0.5;

    seed_img = iftOGRID(objsm, mask, k, perc, thr);

    iftDestroyImage(&objsm);
    iftDestroyImage(&mask);

    // 4. OISF Segmentation ----------------------------------------------------
    int iters;
    float alpha, beta, gamma;

    if(UAIArgsExists(argc, argv, "alpha"))
    {    
        PARAM = UAIArgsGet(argc, argv, "alpha");
        if(PARAM == NULL) 
            iftError("No alpha value was given!", "main");
        else alpha = atof(PARAM);
    
        if(alpha <= 0.0) iftError("Invalid alpha value!", "main");
    }
    else alpha = 0.5;

    if(UAIArgsExists(argc, argv, "beta"))
    {
        PARAM = UAIArgsGet(argc, argv, "beta");
        if(PARAM == NULL) 
            iftError("No beta value was given!", "main");
        else beta = atof(PARAM);
    
        if(beta <= 0.0) iftError("Invalid beta value!", "main");
    }
    else beta = 12;

    if(UAIArgsExists(argc, argv, "gamma"))
    {
        PARAM = UAIArgsGet(argc, argv, "gamma");
        if(PARAM == NULL) 
            iftError("No gamma value was given!", "main");
        else gamma = atof(PARAM);
    
        if(gamma <= 0.0) iftError("Invalid gamma value!", "main");
    }
    else gamma = 2.0;

    if(UAIArgsExists(argc, argv, "iters"))
    {
        PARAM = UAIArgsGet(argc, argv, "iters");
        if(PARAM == NULL)
            iftError("No number of iterations was given!", "main");
        else iters = atoi(PARAM);
    
        if(iters < 1) iftError("Invalid number of iterations!", "main");
    }
    else iters = 10;
    iftOISF(graph, seed_img, alpha, beta, gamma, iters);

    iftDestroyImage(&seed_img);

    // 5. Write Labels ---------------------------------------------------------
    iftImage *labels;

    labels = iftIGraphLabel(graph);

    iftWriteImageByExt(labels, UAIArgsGet(argc, argv, "labels"));

    if(UAIArgsExists(argc, argv, "ovlay"))
    {
        int norm_value;
        iftColor RGB, YCbCr;
        iftAdjRel *A;
        iftImage *added;

        PARAM = UAIArgsGet(argc, argv, "ovlay");
        if(PARAM == NULL)
            iftError("No overlayed image path was given!", "main");
        
        A = iftCircular(1.0);

        added = iftAddValue(labels, 1);

        norm_value = iftNormalizationValue(iftMaximumValue(img));
        RGB.val[0] = RGB.val[1] = RGB.val[2] = 0;
        YCbCr = iftRGBtoYCbCr(RGB, norm_value);

        iftDrawBorders(img, labels, A, YCbCr, A);
        iftWriteImageByExt(img, PARAM);

        iftDestroyImage(&added);
        iftDestroyAdjRel(&A);
    }

    iftDestroyImage(&img);
    iftDestroyImage(&labels);
    iftDestroyIGraph(&graph);

    return EXIT_SUCCESS;
}