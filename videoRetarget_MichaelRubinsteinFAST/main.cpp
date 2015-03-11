/*
*	Copyright (C)   Lyq root#lyq.me
*	File Name     : cartoonResizing
*	Creation Time : 2015-3-9 16:25:00 UTC+8
*	Environment   : Windows8.1-64bit VS2013 OpenCV2.4.9
*	Homepage      : http://www.lyq.me
*/

#ifndef OPENCV
#define OPENCV
#include <opencv2\opencv.hpp>
#include <opencv2\video\video.hpp>
using namespace cv;
#endif // !OPENCV

#ifndef STD
#define STD
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <queue>
#include <ctime>
using namespace std;
#define INF 2000000000
#define sqr(_x) ( (_x) * (_x) )
struct typeEdge {
	int x, y, w, next;
	bool flag;
};
#endif // !STD

void help( void );
bool readVideo( VideoCapture &cap, const char *videoName, vector<Mat> &frames );
void buildGraph( vector<Mat> &frames, vector<bool> &bandNodes, vector<int> &num2pos, vector<int> &edgeHead, vector<typeEdge> &edge );
void maxFlow( vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge );
void surfaceCarving( vector<Mat> &frames, int surfaceDeletedCount, int pyramidIndex, vector<int> &num2pos, vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge, vector< vector<int> > &connectS );
void settleBand( vector< vector<int> > &connectS, vector<bool> &bandNodes, Size pyramidSize, int frameCount );
bool writeVideo( const char *videoName );

inline int txy2pos( int t, int x, int y, int frameCount, const Size &frameSize );
inline bool pos2txy( int num, int &t, int &x, int &y, int N, int frameCount, const Size &frameSize );
inline void buildEdge( vector<int> &edgeHead, vector<typeEdge> &edge, int x, int y, int w, bool flag = false );
int bfsDinic( vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge );
int dfsDinic( int nowP, int minFlow, vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge );


int main( void ) {

	help( );

	int funcType = 0;
	char *processVideo = "test2";
	int pyramidLayer = 5;
	int widthDeleted = 50;

	if ( funcType == 0 ) {

		VideoCapture cap;
		vector<Mat> frames;
		char videoName[100];
		strcpy( videoName, processVideo );
		strcat( videoName, "In.avi" );
		bool state = readVideo( cap, videoName, frames );
		if ( !state ) return -2;
		
		vector< vector<Mat> > pyramidFrames;
		vector< vector<bool> > pyramidBand;
		vector<int> edgeHead;
		vector<typeEdge> edge;
		
		int surfaceDeletedCount = 0;
		 
		for ( int pyramidIndex = 0; pyramidIndex < pyramidLayer; pyramidIndex++ ) {

			if ( pyramidIndex != 0 ) {
				for ( int i = 0; i < (int)frames.size(); i++ ) {
					resize( frames[i], frames[i], Size( ), 0.5, 0.5 );
				}
			}
			int nodeCount = (frames[0].cols + 1) * frames[0].rows * frames.size( ) + 2;
			if ( pyramidIndex == pyramidLayer-1 ) {
				pyramidBand.push_back( vector<bool>( nodeCount, true ) );
			} else {
				pyramidBand.push_back( vector<bool>( nodeCount, false ) );
			}
			
			char bmpName[100];
			for ( int i = 0; i < (int)frames.size(); i++ ) {
				sprintf( bmpName, "inResource//pyramid%d//%d.bmp", pyramidIndex, i );
				imwrite( bmpName, frames[i] );
			}
			pyramidFrames.push_back( frames );
		}

		while ( surfaceDeletedCount < widthDeleted ) {

			int startSurfaceTime = clock();
			surfaceDeletedCount++;
			cout << "\n>>> Start curving surface " << surfaceDeletedCount << endl;

			for ( int pyramidIndex = pyramidLayer-1; pyramidIndex >= 0; pyramidIndex-- ) {

				cout << " >> In pyramid Index " << pyramidIndex << endl;
				int startPyramidTime = clock();

				vector<int> tag;
				vector<int> num2pos;

				buildGraph( pyramidFrames[pyramidIndex], pyramidBand[pyramidIndex], num2pos, edgeHead, edge );
				maxFlow( tag, edgeHead, edge );

				int frameCount = pyramidFrames[pyramidIndex].size();
				Size nextPyramidSize;
				if ( pyramidIndex != 0 ) {
					nextPyramidSize = Size( pyramidFrames[pyramidIndex - 1][0].cols + 1, pyramidFrames[pyramidIndex - 1][0].rows );
				} else {
					nextPyramidSize = Size( (pyramidFrames[0][0].cols << 1) + 1, pyramidFrames[0][0].rows << 1 );
				}
				vector< vector<int> > connectS( nextPyramidSize.height, vector<int>(frameCount, -1) );
				surfaceCarving( pyramidFrames[pyramidIndex], surfaceDeletedCount, pyramidIndex, num2pos, tag, edgeHead, edge, connectS );
				if ( pyramidIndex != 0 ) settleBand( connectS, pyramidBand[pyramidIndex - 1], nextPyramidSize, frameCount );

				int endPyramidTime = clock();
				int deltaPyramidTime = (endPyramidTime - startPyramidTime + 500 ) / 1000;
				printf( " << In pyramid Time used : %d min %d sec\n", deltaPyramidTime / 60, deltaPyramidTime % 60 );
			}

			int endSurfaceTime = clock( );
			int deltaPyramidTime = (endSurfaceTime - startSurfaceTime + 500 ) / 1000;
			printf( "<<< Deleted surface Time used : %d min %d sec\n", deltaPyramidTime / 60, deltaPyramidTime % 60 );
		}
		
	} else {

		char *videoName = processVideo;
		strcat( videoName, "Out.avi" );
		bool state = writeVideo( videoName );
		if ( !state ) return -2;
	}

	return 0;
}

void help( void ) {

	printf( "===	Copyright (C) Lyq root#lyq.me\n"
			"===	File Name     : cartoonResizing\n"
			"===	Creation Time : 2015-3-9 16:25:00 UTC+8\n"
			"===	Environment   : Windows8.1-64bit VS2013 OpenCV2.4.9\n"
			"===	Homepage      : http://www.lyq.me\n"
			"===	\n"
			"===	This program demostrated a simple method of cartoon resizing.\n"
			"===	It learns the pixels' Energy and calculates the min surface Curve.\n"
			"===	Reference: Improved Seam Carving for Video Retargeting\n"
			"		\n" );
}

bool readVideo( VideoCapture &cap, const char *videoName, vector<Mat> &frames ) {

	if ( !cap.open( videoName ) ) {
		cout << "!!! Could not open the output video for reading!" << endl;
		return false;
	}

	Mat inputFrame;
	int count = 0;

	//int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC));
	//cout << ex;

	while ( cap.read( inputFrame ) ) {

		cvtColor( inputFrame, inputFrame, CV_RGB2GRAY );
		medianBlur( inputFrame, inputFrame, 3 );
		frames.push_back( inputFrame.clone( ) );

		//imshow( "originVideo", inputFrame );
		//waitKey( 50 );
		
		count++;
		if ( count == 200 ) break;
	}
	return true;
}

void buildGraph( vector<Mat> &frames, vector<bool> &bandNodes, vector<int> &num2pos, vector<int> &edgeHead, vector<typeEdge> &edge ) {

	int frameCount = frames.size( );
	Size frameSize = Size( frames[ 0 ].cols+1, frames[ 0 ].rows );
	cout << "  > frameHeight : " << frameSize.height << " px" << endl;
	cout << "  > frameWidth : " << frameSize.width << " px" << endl;
	cout << "  > frameCount : " << frameCount << endl;

	int N = frameCount * frameSize.width * frameSize.height + 2;
	int M = 1;
	num2pos.push_back( 0 );
	vector<int> pos2num( N, -1 );
	vector< vector<int> > connectS( frameSize.height, vector<int>( frameCount, frameSize.width - 1 ) );
	vector< vector<int> > connectT( frameSize.height, vector<int>( frameCount, 0 ) ); 
	for ( int i = 1; i < N-1; i++ ) {

		if ( !bandNodes[i] ) continue;

		int t, x, y;
		if ( !pos2txy( i, t, x, y, N, frameCount, frameSize ) ) continue;
		connectS[y][t] = min( connectS[y][t], x );
		if ( x != frameSize.width - 1 ) connectT[y][t] = max( connectT[y][t], x );
		pos2num[i] = M++;
		num2pos.push_back( i );
	}
	
	N = M + 1;
	int S = 0, T = N - 1;

	edgeHead.clear( );
	edge.clear( );
	edgeHead = vector<int>( N, -1 );
	cout << "  > NodeSize : " << N << endl;

	int p0, p1, p2, w;
	for ( int y = 0; y < frameSize.height; y++ ) {
		for ( int t = 0; t < frameCount; t++ ) {
			
			p0 = pos2num[txy2pos( t, connectS[y][t], y, frameCount, frameSize )];
			buildEdge( edgeHead, edge, S, p0, INF );
			buildEdge( edgeHead, edge, p0, S, 0 );

			p0 = pos2num[txy2pos( t, frameSize.width - 1, y, frameCount, frameSize )];
			buildEdge( edgeHead, edge, p0, T, INF );
			buildEdge( edgeHead, edge, T, p0, 0 );
		}
	}
	//cout << "After S & T edge.size() = " << edge.size() << endl;

	// XY-Plane
	for ( int t = 0; t < frameCount; t++ ) {

		// LR
		for ( int y = 0; y < frameSize.height; y++ ) {

			uchar *rowData = frames[ t ].ptr<uchar>( y );
			for ( int x = connectS[y][t]; x <= connectT[y][t]; x++ ) {

				p0 = pos2num[txy2pos( t, x, y, frameCount, frameSize )];
				if ( x < connectT[y][t] ) {
					p1 = pos2num[txy2pos( t, x + 1, y, frameCount, frameSize )];
				} else {
					p1 = pos2num[txy2pos( t, frameSize.width - 1, y, frameCount, frameSize )];
				}
				p2 = txy2pos( t, x - 1, y, frameCount, frameSize );
				if ( x < (frameSize.width - 2) && p2 != -1 ) {
					w = abs( rowData[ x + 1 ] - rowData[ x - 1 ] );
				} else {
					if ( p2 != -1 ) {
						w = abs( rowData[x] - rowData[x - 1] );
					} else {
						w = abs( rowData[x + 1] - rowData[x] );
					}
				}

				buildEdge( edgeHead, edge, p0, p1, w, true );
				buildEdge( edgeHead, edge, p1, p0, INF );
			}
		}

		for ( int y = 1; y < frameSize.height; y++ ) {

			uchar *rowData0 = frames[ t ].ptr<uchar>( y - 1 );
			uchar *rowData1 = frames[ t ].ptr<uchar>( y );
			for ( int x = connectS[y][t]; x <= connectT[y][t]; x++ ) {
				 
				p0 = pos2num[txy2pos( t, x, y, frameCount, frameSize )];
				p1 = pos2num[txy2pos( t, x, y - 1, frameCount, frameSize )];
				if ( p1 == -1 ) {
					if ( x == connectS[y][t] ) continue;
					p1 = pos2num[txy2pos( t, frameSize.width - 1, y - 1, frameCount, frameSize )];
				}

				// LU
				p2 = txy2pos( t, x - 1, y, frameCount, frameSize );
				if ( p2 != -1 ) {

					w = abs( rowData0[x] - rowData1[x - 1] );
					buildEdge( edgeHead, edge, p0, p1, w );
					buildEdge( edgeHead, edge, p1, p0, 0 );
					p2 = pos2num[p2];
					if ( p2 != -1 ) { 
						buildEdge( edgeHead, edge, p1, p2, INF );
						buildEdge( edgeHead, edge, p2, p1, 0 );
					}
				}
				

				// LD
				p2 = txy2pos( t, x - 1, y - 1, frameCount, frameSize );
				if ( p2 != -1 ) {

					w = abs( rowData0[x - 1] - rowData1[x] );
					buildEdge( edgeHead, edge, p1, p0, w );
					buildEdge( edgeHead, edge, p0, p1, 0 );
					p2 = pos2num[p2];
					if ( p2 != -1 ) {
						buildEdge( edgeHead, edge, p0, p2, INF );
						buildEdge( edgeHead, edge, p2, p0, 0 );
					}
				}
			}
		}
	}

	//cout << "After XY-Plane edge.size() = " << edge.size() << endl;

	// XT-Plane
	for ( int y = 0; y < frameSize.height; y++ ) {

		for ( int t = 1; t < frameCount; t++ ) {

			uchar *rowData0 = frames[ t - 1 ].ptr<uchar>( y );
			uchar *rowData1 = frames[ t ].ptr<uchar>( y );
			for ( int x = connectS[y][t]; x <= connectT[y][t]; x++ ) {

				p0 = pos2num[txy2pos( t, x, y, frameCount, frameSize )];
				p1 = pos2num[txy2pos( t - 1, x, y, frameCount, frameSize )];
				if ( p1 == -1 ) {
					if ( x == connectS[y][t] ) continue;
					p1 = pos2num[txy2pos( t - 1, frameSize.width - 1, y, frameCount, frameSize )];
				}

				// LU
				p2 = txy2pos( t, x - 1, y, frameCount, frameSize );
				if ( p2 != -1 ) {

					w = abs( rowData0[x] - rowData1[x - 1] );
					buildEdge( edgeHead, edge, p0, p1, w );
					buildEdge( edgeHead, edge, p1, p0, 0 );
					p2 = pos2num[p2];
					if ( p2 != -1 ) {
						buildEdge( edgeHead, edge, p1, p2, INF );
						buildEdge( edgeHead, edge, p2, p1, 0 );
					}
				}

				// LD
				p2 = txy2pos( t - 1, x - 1, y, frameCount, frameSize );
				if ( p2 != -1 ) {
					
					w = abs( rowData0[x - 1] - rowData1[x] );
					buildEdge( edgeHead, edge, p1, p0, w );
					buildEdge( edgeHead, edge, p0, p1, 0 );
					p2 = pos2num[p2];
					if ( p2 != -1 ) {
						buildEdge( edgeHead, edge, p0, p2, INF );
						buildEdge( edgeHead, edge, p2, p0, 0 );
					}
				}
			}
		}
	}
	//cout << "After XT-Plane edge.size() = " << edge.size() << endl;
	cout << "  > edgeSize : " << edge.size( ) << endl;
}

inline int txy2pos( int t, int x, int y, int frameCount, const Size &frameSize ) {

	if ( t < 0 || t >= frameCount ) return -1;
	if ( x < 0 || x >= frameSize.width ) return -1;
	if ( y < 0 || y >= frameSize.height ) return -1;
	return t * frameSize.width * frameSize.height + y * frameSize.width + x + 1;

}

inline bool pos2txy( int num, int &t, int &x, int &y, int N, int frameCount, const Size &frameSize ) {

	if ( num <= 0 || num >= N - 1 ) return false;

	num--;
	int temp = frameSize.width * frameSize.height;
	t = num / temp;
	num = num - t * temp;
	y = num / frameSize.width;
	num = num - y * frameSize.width;
	x = num;

	return true;
}

inline void buildEdge( vector<int> &edgeHead, vector<typeEdge> &edge, int x, int y, int w, bool flag ) {

	typeEdge oneEdge;
	oneEdge.x = x;
	oneEdge.y = y;
	oneEdge.w = w;
	oneEdge.flag = flag;
	oneEdge.next = edgeHead[ x ];
	edgeHead[ x ] = edge.size( );
	edge.push_back( oneEdge );
}

void maxFlow( vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge ) {

	long long ans = 0;
	
	while ( bfsDinic( tag, edgeHead, edge ) ) {

		int tans = 1;
		while ( tans > 0 ) {

			tans = dfsDinic( 0, 0x7fffffff, tag, edgeHead, edge );
			ans += tans;
		}
	}
	cout << "  > maxFlow : " << ans << endl;
}

int bfsDinic( vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge ) {

	queue<int> que;
	tag = vector<int>( edgeHead.size(), -1 );
	tag[0] = 0;
	que.push( 0 );
	while ( !que.empty() ) {

		int nowP = que.front();
		que.pop();
		for ( int p = edgeHead[nowP]; p != -1; p = edge[p].next ) {

			int nextP = edge[p].y;
			if ( tag[nextP] == -1 && edge[p].w > 0 ) {
				tag[nextP] = tag[nowP] + 1;
				que.push( nextP );
			}
		}
	}
	int N = tag.size();
	if ( tag[N - 1] > 0 ) {
		return 1;
	} else {
		return 0;
	}
}

int dfsDinic( int nowP, int minFlow, vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge ) {

	//cout << nowP << " " << tag[nowP] << " " << minFlow << endl;
	static vector<bool> viewedP;
	if ( tag[nowP] == 0 ) viewedP = vector<bool>( tag.size(), false );
	viewedP[nowP] = true;
	if ( minFlow == 0 ) return 0;
	if ( nowP == tag.size() - 1 ) return minFlow;

	for ( int p = edgeHead[nowP]; p != -1; p = edge[p].next ) {

		int nextP = edge[p].y;
		if ( tag[nextP] != tag[nowP] + 1 ) continue;
		if ( edge[p].w <= 0 ) continue;
		if ( viewedP[nextP] ) continue;

		int flow = dfsDinic( nextP, min( minFlow, edge[p].w ), tag, edgeHead, edge );
		if ( flow > 0 ) {
			edge[p].w -= flow;
			edge[p + 1].w += flow;
			return flow;
		}
	}
	return 0;
}

void surfaceCarving( vector<Mat> &frames, int surfaceDeletedCount, int pyramidIndex, vector<int> &num2pos, vector<int> &tag, vector<int> &edgeHead, vector<typeEdge> &edge, vector< vector<int> > &connectS ) {

	bfsDinic( tag, edgeHead, edge );

	
	int frameCount = frames.size();
	Size frameSize = Size( frames[0].cols+1, frames[0].rows );
	int frameType = frames[0].type();
	int N = edgeHead.size();
	int M = frameSize.width * frameSize.height * frameCount + 2;
	bool isRemoved;
	vector<Mat> frames2;
	vector<Mat> frames3;

	for ( int i = 0; i < frameCount; i++ ) {
		frames2.push_back( Mat( frameSize.height, frameSize.width - 2, frameType ) );
		frames3.push_back( Mat( frameSize.height, frameSize.width - 1, frameType ) );
		//imshow( "test", frames[i] );
		//waitKey( 50 );
	}

	for ( int i = 1; i < N; i++ ) {

		if ( tag[i] == -1 ) continue;
		isRemoved = false;
		for ( int p = edgeHead[i]; p != -1; p = edge[p].next ) {

			//cout << edge[p].flag << " " << tag[edge[p].y] << endl;
			if ( edge[p].flag && tag[edge[p].y] == -1 ) {
				isRemoved = true;
				break;
			}
		}

		if ( !isRemoved ) continue;

		int t0, x0, y0;
		if ( !pos2txy( num2pos[i], t0, x0, y0, M, frameCount, frameSize ) ) continue;

		uchar *rowData0, *rowData1;
		rowData0 = frames[t0].ptr<uchar>( y0 );
		rowData1 = frames2[t0].ptr<uchar>( y0 );
		for ( int x = 0; x < x0; x++ ) rowData1[x] = rowData0[x];
		for ( int x = x0 + 1; x < frameSize.width - 1; x++ ) rowData1[x - 1] = rowData0[x];

		rowData1 = frames3[t0].ptr<uchar>( y0 );
		for ( int x = 0; x < frameSize.width - 1; x++ ) rowData1[x] = rowData0[x];
		rowData1[x0] = 0;

		connectS[y0 << 1][t0] = x0 << 1;
	}

	char bmpName[100];
	for ( int t = 0; t < frameCount; t++ ) {

		sprintf( bmpName, "outSurface//pyramid%d//%d_%d.bmp", pyramidIndex, surfaceDeletedCount, t );
		imwrite( bmpName, frames3[t] );
		imshow( "3", frames3[t] );

		sprintf( bmpName, "outResult//pyramid%d//%d_%d.bmp", pyramidIndex, surfaceDeletedCount, t );
		imwrite( bmpName, frames2[t] );
		imshow( "2", frames2[t] );
		waitKey( 100 );

	}
	waitKey( 0 );

	frames = frames2;
}

void settleBand( vector< vector<int> > &connectS, vector<bool> &bandNodes, Size pyramidSize, int frameCount ) {

	for ( int y = 1; y < pyramidSize.height; y += 2 ) {
		for ( int t = 0; t < frameCount; t++ ) {
			if ( y + 1 < pyramidSize.height ) {
				connectS[y][t] = (connectS[y - 1][t] + connectS[y + 1][t]) / 2;
			} else {
				connectS[y][t] = connectS[y - 1][t];
			}
		}
	}
	int p0;
	for ( int y = 0; y < pyramidSize.height; y++ ) {
		for ( int t = 0; t < frameCount; t++ ) {
			for ( int x = max(0, connectS[y][t]); x < min(pyramidSize.width, connectS[y][t]+5); x++ ) {
				
				p0 = txy2pos( t, x, y, frameCount, pyramidSize );
				bandNodes[p0] = true;
			}
			p0 = txy2pos( t, pyramidSize.width - 1, y, frameCount, pyramidSize );
			bandNodes[p0] = true;
		}
	}
	/*
	for ( int y = 0; y < pyramidSize.height; y++ ) {
		for ( int t = 0; t < frameCount; t++ ) {
			printf( "%d ", connectS[y][t] );
		}
		printf( "\n" );
	}

	for ( int y = 0; y < pyramidSize.height; y+=2 ) {
		for ( int t = 1; t < frameCount; t+=2 ) {
			if ( t + 1 < frameCount ) {	
				connectS[y][t] = (connectS[y][t - 1] + connectS[y][t + 1]) / 2;
			} else {
				connectS[y][t] = connectS[y][t - 1];
			}
		}
	}
	for ( int y = 0; y < pyramidSize.height; y++ ) {
		for ( int t = 0; t < frameCount; t++ ) {
			printf( "%d ", connectS[y][t] );
		}
		printf( "\n" );
	}
	
	for ( int y = 1; y < pyramidSize.height; y += 2 ) {
		for ( int t = 1; t < frameCount; t += 2 ) {
			if ( t + 1 < frameCount ) {
				connectS[y][t] = (connectS[y][t - 1] + connectS[y][t + 1]) / 2;
			} else {
				connectS[y][t] = connectS[y][t - 1];
			}
		}
	}
	*/

	
}

bool writeVideo( const char *videoName ) {

	VideoWriter outputVideo;
	outputVideo.open( videoName, CV_FOURCC( 'I', 'Y', 'U', 'V' ), 25, Size( 505, 250 ) );

	if ( !outputVideo.isOpened( ) ) {

		cout << "!!! Could not open the output video for writing!" << endl;
		return false;
	}

	char inFrameName[ 100 ], outFrameName[ 100 ], combineFrameName[ 100 ];
	Mat inFrame, outFrame, outFrame1, combineFrame;
	uchar *rowData0, *rowData1;
	namedWindow( "combineVideo" );

	for ( int i = 0; i < 20; i++ ) {

		sprintf( inFrameName, "inResource//%d.bmp", i );
		inFrame = imread( inFrameName );
		cvtColor( inFrame, inFrame, CV_RGB2GRAY );
		resize( inFrame, inFrame, Size( ), 10, 10 );
		sprintf( outFrameName, "outResult//10_%d.bmp", i );
		outFrame = imread( outFrameName );
		cvtColor( outFrame, outFrame, CV_RGB2GRAY );
		resize( outFrame, outFrame, Size( ), 10, 10 );

		sprintf( outFrameName, "outResult1//10_%d.bmp", i );
		outFrame1 = imread( outFrameName );
		cvtColor( outFrame1, outFrame1, CV_RGB2GRAY );
		resize( outFrame1, outFrame1, Size( ), 10, 10 );

		combineFrame = Mat::zeros( inFrame.rows, inFrame.cols * 3 + 10, inFrame.type( ) );

		for ( int y = 0; y < inFrame.rows; y++ ) {

			rowData0 = combineFrame.ptr<uchar>( y );
			rowData1 = inFrame.ptr<uchar>( y );
			for ( int x = 0; x < inFrame.cols; x++ ) rowData0[ x ] = rowData1[ x ];

			rowData1 = outFrame.ptr<uchar>( y );
			for ( int x = 0; x < outFrame.cols; x++ ) rowData0[ x + inFrame.cols + 5 ] = rowData1[ x ];

			rowData1 = outFrame1.ptr<uchar>( y );
			for ( int x = 0; x < outFrame.cols; x++ ) rowData0[ x + inFrame.cols * 2 + 10 ] = rowData1[ x ];

		}

		sprintf( combineFrameName, "combineFrame//%d.bmp", i );
		imwrite( combineFrameName, combineFrame );
		outputVideo << combineFrame;
		imshow( "combineVideo", combineFrame );
		waitKey( 100 );
	}

	waitKey( 0 );

	return true;
}