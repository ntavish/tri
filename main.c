#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#define OUT "output"
#define BARS "bars"

#define SWAP(x,y) t=x;x=y;y=t;
IplImage *t;

////////////////
// PARAMS
//blur
int blur_param=4;
//thresh
int lowthresh=75;
int highthresh=255;
//findcontour
int ct1=70, ct2=120;
//findcorners
int blockSize=3;
//every_contour
int every=10, mincontour=20;
////////////////
int k=0;
IplImage *in=NULL, *out=NULL;//in and out are 1 channel images
IplImage *orig, *orighsv, *origH, *origS, *origV;
IplImage *temp;

CvMemStorage *storage;

CvMemStorage *trianglestore;//delaunay
CvRect rect;//for delaunay
CvSubdiv2D* subdiv;

CvSeq *contours;


void every_contour(CvSeq *contours, IplImage *im)
{
	CvSeq *current=contours;

	while(current!=NULL)
	{
		int total=current->total;
		if(total<mincontour)
		{
			current=current->h_next;
			continue;
		}
		int i;

		FILE *poly;
		poly = fopen("vert.poly", "wa");

		int num;
		char n[30];
		sprintf(n, "%d", total/every);
		write(poly, n, strlen(n));
		write(poly, " 2 0 0", 6);

		for( i=0; i<total; i+=every )
		{
			CvPoint* p = (CvPoint*)cvGetSeqElem ( current, i );

			char ptstring[100];
			sprintf(ptstring, "\n%d %d %d", i/every, p->x, p->y);
			write(poly, ptstring, strlen(ptstring));

			//printf(“(%d,%d)\n”, p->x, p->y );
			//cvCircle(im, *p, 1, cvScalar(255,0,0,0), 1, 8, 0);
			cvSubdivDelaunay2DInsert(subdiv, cvPoint2D32f(p->x,p->y));
		}

		write(poly, "\n0\n0", 4);

		close(poly);exit(0);

		current=current->h_next;
	}
}


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
                    CV_CHAIN_APPROX_NONE, cvPoint(0,0) );

    //release all the temporary images
    cvReleaseImage( &gray );
    cvReleaseImage( &tgray );
    cvReleaseImage( &timg );

}

void findcorners(IplImage *in, IplImage *out)
{
	cvCornerHarris(
		in,
		out,
		blockSize,
		3,
		0.04 );
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

void draw_subdiv_edge( IplImage* img, CvSubdiv2DEdge edge, CvScalar color )
{
    CvSubdiv2DPoint* org_pt;
    CvSubdiv2DPoint* dst_pt;
    CvPoint2D32f org;
    CvPoint2D32f dst;
    CvPoint iorg, idst;

    org_pt = cvSubdiv2DEdgeOrg(edge);
    dst_pt = cvSubdiv2DEdgeDst(edge);

    if( org_pt && dst_pt )
    {
        org = org_pt->pt;
        dst = dst_pt->pt;

        iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
        idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

        cvLine( img, iorg, idst, color, 1, CV_AA, 0 );
    }
}

void draw_subdiv( IplImage* img, CvSubdiv2D* subdiv,
                  CvScalar delaunay_color)
{
    CvSeqReader  reader;
    int i, total = subdiv->edges->total;
    int elem_size = subdiv->edges->elem_size;

    cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );

    for( i = 0; i < total; i++ )
    {
        CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

        if( CV_IS_SET_ELEM( edge ))
        {
            draw_subdiv_edge( img, (CvSubdiv2DEdge)edge, delaunay_color );
        }

        CV_NEXT_SEQ_ELEM( elem_size, reader );
    }
}


void draw(int dummy)
{
	//delaunay
	cvClearMemStorage(storage);
	subdiv=cvCreateSubdivDelaunay2D(rect,trianglestore);

	blur(origV, out);
	SWAP(in,out);
	thresh(in, out);
	findContours(out, storage, &contours);

	cvMerge(origH, origS, out, NULL, temp);
	cvCvtColor( temp, temp, CV_HSV2RGB );

	every_contour(contours, temp);
	drawContour(temp, contours);
	SWAP(in,out);

	draw_subdiv(temp,subdiv, cvScalar(255,255,255,255));
		if(k==0)
		{
		cvNot(in,out);
		//k=1;
		}
		else{}
	cvClearMemStorage(trianglestore);
	//findcorners(origH,out);   //needs 32bit float image

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
	cvCreateTrackbar("blocksize", BARS, &blockSize, 255, draw);
	cvCreateTrackbar("skipevery", BARS, &every, 255, draw);
	cvCreateTrackbar("mincontour", BARS, &mincontour, 255, draw);

    storage = cvCreateMemStorage(0);
    trianglestore = cvCreateMemStorage(0);

	if(argc > 1 )
	{
		orig = cvLoadImage( (const char*)argv[1] , CV_LOAD_IMAGE_COLOR);
	}
	else
	{
		orig = cvLoadImage( "input.jpg" , CV_LOAD_IMAGE_COLOR);
	}

	rect=cvRect(0,0,orig->width, orig->height);//delaunay
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

	cvShowImage( OUT, out);

	cvWaitKey(0);

    return 0;
}
