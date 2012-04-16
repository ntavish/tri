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
//blur
int blur_param=0;
//thresh
int lowthresh=0;
int highthresh=255;
//findcontour
int ct1=70, ct2=120;
////////////////

IplImage *in=NULL, *out=NULL;//in and out are 1 channel images
IplImage *orig, *orighsv, *origH, *origS, *origV;
IplImage *temp;

CvMemStorage *storage;

CvSeq *contours;


void drawContour(IplImage * image, CvSeq *cont)
{
	cvDrawContours( image, cont, cvScalar(255,255,255,0), cvScalar(255,255,255,0), 1, 1, 8 , cvPoint(0,0) );
}

void findContours( IplImage* img, CvMemStorage* storage, CvSeq **contours)
{
    //for findContour function
    IplImage* timg  =NULL;
    IplImage* gray  =NULL;
    IplImage* tgray =NULL;

    CvSize sz = cvSize( img->width, img->height );

	// make a copy of input image
	gray = cvCreateImage( sz, img->depth, 1 );
	timg = cvCreateImage( sz, img->depth, 1 );
	tgray = cvCreateImage( sz, img->depth, 1 );

	cvSetImageCOI(img,1);
    cvCopy( img, timg,NULL );
	cvSetImageCOI(img,0);

    cvCopy( timg, tgray, 0 );

    cvCanny( tgray, gray, ct1, ct2, 5 );
    // holes between edge segments
    cvDilate( gray, gray, 0, 2 );

    cvFindContours( gray, storage, contours,
                    sizeof(CvContour),CV_RETR_LIST,
                    CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

    //release all the temporary images
    cvReleaseImage( &gray );
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
	cvClearMemStorage(storage);

	blur(origV, out);
	SWAP(in,out);
	thresh(in, out);
	findContours(out, storage, &contours);

	cvMerge(origH, origS, out, NULL, temp);
	cvCvtColor( temp, temp, CV_HSV2RGB );

	drawContour(temp, contours);

	cvShowImage(OUT, temp);
}

int main(int argc, char *argv[])
{
    cvNamedWindow( OUT, 0 );
    cvNamedWindow( BARS, 0);

	cvCreateTrackbar("blur", BARS, &blur_param, 15, draw);
	cvCreateTrackbar("lowthresh", BARS, &lowthresh, 255, draw);
	cvCreateTrackbar("highthresh", BARS, &highthresh, 255, draw);
	cvCreateTrackbar("ct1", BARS, &ct1, 255, draw);
	cvCreateTrackbar("ct2", BARS, &ct2, 255, draw);

    storage = cvCreateMemStorage(0);

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

	temp=cvCreateImage(cvGetSize(orig), orig->depth, 3);

	origH=cvCreateImage(cvGetSize(orig), orig->depth, 1);
	origS=cvCreateImage(cvGetSize(orig), orig->depth, 1);
	origV=cvCreateImage(cvGetSize(orig), orig->depth, 1);

	orighsv=cvCreateImage(cvGetSize(orig), orig->depth, 3);
	cvCvtColor( orig, orighsv, CV_RGB2HSV);
	cvSplit( orighsv,origH, origS,origV, NULL );

	draw(0);

	cvShowImage(OUT, out);

	cvWaitKey(0);

    return 0;
}
