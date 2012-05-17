#include "cv.h"
#include "cxtypes.h"
#include "cxcore.h"
#include "cvtypes.h"
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
int blur_param=4;
//thresh
int lowthresh=75;
int highthresh=255;
//findcontour
int ct1=70, ct2=120;
//findcorners
int blockSize=3;
//every_contour
int every=27, mincontour=10, maxcontour=2000;
////////////////

IplImage *in=NULL, *out=NULL;//in and out are 1 channel images
IplImage *mask;//for triangles
IplImage *orig, *orighsv, *origH, *origS, *origV;
IplImage *temp;
IplImage *final;

//IplImage *

CvMemStorage *storage;

CvMemStorage *trianglestore;//delaunay
CvRect rect;//for delaunay
CvSubdiv2D* subdiv;

CvSeq *contours;

void draw_subdiv_point( IplImage* img, CvPoint2D32f fp, CvScalar color )
{
    cvCircle( img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 3, color, CV_FILLED, 8, 0 );
}

void draw_subdiv_facet( IplImage* img, CvSubdiv2DEdge edge )
{
    CvSubdiv2DEdge t = edge;
    int i, count = 0;
    CvPoint* buf = 0;

    // count number of edges in facet
    do
    {
        count++;
        t = cvSubdiv2DGetEdge( t, CV_NEXT_AROUND_LEFT );
    } while (t != edge );

    buf = (CvPoint*)malloc( count * sizeof(buf[0]));

    // gather points
    t = edge;
    for( i = 0; i < count; i++ )
    {
        CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg( t );
        CvPoint2D32f pt32f = pt->pt;// to 32f point
		CvPoint p = cvPointFrom32f(pt32f); // to an integer point

        if( !pt || p.x<0 || p.x > temp->width || p.y < 0 || p.y > temp->height) break;
        buf[i] = cvPoint( cvRound(pt->pt.x), cvRound(pt->pt.y));
        t = cvSubdiv2DGetEdge( t, CV_NEXT_AROUND_LEFT );
    }

    if( i == count )
    {
    	cvZero(mask);

        CvSubdiv2DPoint* pt = cvSubdiv2DEdgeDst( cvSubdiv2DRotateEdge( edge, 0 ));
        //fill mask with 1 triangle
        cvFillConvexPoly( mask, buf, count, CV_RGB(255,255,255), CV_AA, 0 );
        CvScalar col=cvAvg(orig, mask);

        cvFillConvexPoly( img, buf, count, col, CV_AA, 0 );
        cvPolyLine( img, &buf, &count, 1, 1, CV_RGB(255,255,255), 1, CV_AA, 0);
        //draw_subdiv_point( img, pt->pt, CV_RGB(0,0,0));
    }
    free( buf );
}

void paint_delaunay( CvSubdiv2D* subdiv, IplImage* img )
{
    CvSeqReader  reader;
    int i, total = subdiv->edges->total;
    int elem_size = subdiv->edges->elem_size;

    cvCalcSubdivVoronoi2D( subdiv );

    cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );

    for( i = 0; i < total; i++ )
    {
        CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

        if( CV_IS_SET_ELEM( edge ))
        {
            CvSubdiv2DEdge e = (CvSubdiv2DEdge)edge;
            // self
            draw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 0 ));

            // revself
            draw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 2 ));
        }

        CV_NEXT_SEQ_ELEM( elem_size, reader );
    }
}


void every_contour(CvSeq *contours, IplImage *im)
{
	CvSeq *current=contours;

	while(current!=NULL)
	{
		//delaunay
		cvClearMemStorage(storage);
		subdiv=cvCreateSubdivDelaunay2D(rect,trianglestore);


		int total=current->total;
		if(total>maxcontour|| total<mincontour || (total/every)<3)
		{
			current=current->h_next;
			continue;
		}
		int i;
		for( i=0; i<total; i+=every )
		{
			CvPoint* p = (CvPoint*)cvGetSeqElem ( current, i );
			//printf(“(%d,%d)\n”, p->x, p->y );
			//cvCircle(im, *p, 1, cvScalar(255,0,0,0), 1, 8, 0);
			cvSubdivDelaunay2DInsert(subdiv, cvPoint2D32f(p->x,p->y));

		}

		current=current->h_next;

		//draw_subdiv( temp, subdiv, cvScalar(255,255,255,255));
		cvCalcSubdivVoronoi2D(subdiv);
		paint_delaunay(subdiv, im);

		cvClearMemStorage(trianglestore);

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
    //cvDilate( gray, gray, 0, 2 );

    //cvShowImage(OUT, gray);
    //cvWaitKey(0);

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
		//printf("%d %d     %d %d\n", iorg.x,iorg.y,idst.x,idst.y);
		if(!(iorg.x<0||iorg.y<0||idst.x<0||idst.y<0||iorg.x>img->width||iorg.y>img->height||idst.x>img->width||idst.y>img->height))
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
	blur(origV, out);
	SWAP(in,out);
	thresh(in, out);

	cvZero(final);

	findContours(out, storage, &contours);

	cvMerge(origH, origS, out, NULL, temp);
	cvCvtColor( temp, temp, CV_HSV2RGB );

	every_contour(contours, final);
	//drawContour(temp, contours);
	SWAP(in,out);


	//findcorners(origH,out);   //needs 32bit float image

	cvShowImage(OUT, final);

}

int main(int argc, char *argv[])
{
    cvNamedWindow( OUT, 0 );
    cvNamedWindow( BARS, 0);

	cvCreateTrackbar("blur", BARS, &blur_param, 30, draw);
	cvCreateTrackbar("lowthresh", BARS, &lowthresh, 255, draw);
	cvCreateTrackbar("highthresh", BARS, &highthresh, 255, draw);
	cvCreateTrackbar("ct1", BARS, &ct1, 255, draw);
	cvCreateTrackbar("ct2", BARS, &ct2, 255, draw);
	cvCreateTrackbar("blocksize", BARS, &blockSize, 255, draw);
	cvCreateTrackbar("skipevery", BARS, &every, 500, draw);
	cvCreateTrackbar("mincontour", BARS, &mincontour, 5000, draw);
	cvCreateTrackbar("maxcontour", BARS, &maxcontour, 5000, draw);

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

	mask=cvCreateImage(cvGetSize(orig), orig->depth, 1);
	final=cvCreateImage(cvGetSize(orig), orig->depth, 3);cvZero(final);


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

	cvShowImage(OUT, out);

	while(1){
		char c;
		if((c=cvWaitKey(0))=='q')
			exit(0);
		else if(c=='s')
			cvSaveImage("save.jpg", final, NULL);
		else
				draw(0);}

    return 0;
}
