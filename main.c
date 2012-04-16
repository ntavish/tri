#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>

#define OUT "output"
#define BARS "bars"

#define SWAP(x,y) t=x;x=y;y=t;
IplImage *t;

////////////////
// PARAMS
int blur_param=0;
int lowthresh=0;
int highthresh=255;
////////////////

IplImage *in=NULL, *out=NULL;//in and out are 1 channel images
IplImage *orig, *orighsv;

void findContours( IplImage* img, CvMemStorage* storage, CvSeq **contours, int threshold1, int threshold2)
{
    //for findContour function
    IplImage* timg  =NULL;
    IplImage* gray  =NULL;
    IplImage* pyr   =NULL;
    IplImage* tgray =NULL;

    CvSize sz = cvSize( img->width, img->height );

	// make a copy of input image
	gray = cvCreateImage( sz, img->depth, 1 );
	timg = cvCreateImage( sz, img->depth, 1 );
	pyr = cvCreateImage( cvSize(sz.width/2, sz.height/2), img->depth, 1 );
	tgray = cvCreateImage( sz, img->depth, 1 );

	cvSetImageCOI(img,1);
    cvCopy( img, timg,NULL );
	cvSetImageCOI(img,0);

    //cvSetImageROI( timg, cvRect( 0, 0, sz.width, sz.height ));

    // down-scale and upscale the image to filter out the noise
    cvPyrDown( timg, pyr, 7 );
    cvPyrUp( pyr, timg, 7 );

	cvCopy( timg, tgray, 0 );

    // apply Canny. Take the upper threshold from slider
    // and set the lower to 0 (which forces edges merging)
    cvCanny( tgray, gray, threshold1, threshold2/*param1*/, 5 );
    // holes between edge segments
    cvDilate( gray, gray, 0, 2 );

    cvFindContours( gray, storage, contours,
                    sizeof(CvContour),CV_RETR_LIST,
                    CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

    //release all the temporary images
    cvReleaseImage( &gray );
    cvReleaseImage( &pyr );
    cvReleaseImage( &tgray );
    cvReleaseImage( &timg );

}


void blur(IplImage *in, IplImage *out)
{
	cvSmooth(in, out, CV_GAUSSIAN, 1+blur_param*2, 0, 0, 0);
}

void thresh(IplImage *in, IplImage *out)
{
	cvThreshold(in, out, lowthresh, 0, CV_THRESH_TOZERO);
	cvThreshold(out, out, highthresh, 0, CV_THRESH_TOZERO_INV);
}

void draw(int dummy)
{
	blur(in, out);
	SWAP(in,out);
	thresh(in, out);

	cvShowImage(OUT, out);
}

int main(int argc, char *argv[])
{
    cvNamedWindow( OUT, 0 );
    cvNamedWindow( BARS, 0);

	cvCreateTrackbar("blur", BARS, &blur_param, 15, draw);
	cvCreateTrackbar("lowthresh", BARS, &lowthresh, 255, draw);
	cvCreateTrackbar("highthresh", BARS, &highthresh, 255, draw);
    CvMemStorage *storage = cvCreateMemStorage(0);

	if(argc > 1 )
	{
		orig = cvLoadImage( (const char*)argv[1] , CV_LOAD_IMAGE_COLOR);
	}
	else
	{
		orig = cvLoadImage( "input.jpg" , CV_LOAD_IMAGE_COLOR);
	}

	//  in and out => 1 channel, hue channel
	out=cvCreateImage(cvGetSize(orig), orig->depth, 1);
	in=cvCreateImage(cvGetSize(orig), orig->depth, 1);
	cvCvtColor(orig, in, CV_RGB2GRAY);
	orighsv=cvCreateImage(cvGetSize(orig), orig->depth, orig->nChannels);
	cvCvtColor(orig, orighsv, CV_RGB2HSV);

	draw(0);

	cvShowImage(OUT, out);

	cvWaitKey(0);

    return 0;
}
