// Resizing frequency response and SNR estimation test.

#include <stdio.h>
#include "../avir.h"
#include "../lancir.h"

#include "/libvox/Sources/Core/CWorkerThreadPool.h"
using namespace vox;

#define IS_AVIR 1 // Use AVIR resizer, LancIR otherwise.
#define IS_UPS 1 // Perform upsampling, downsampling otherwise.

#if IS_AVIR
	#define NAME "AVIR29"
#else // IS_AVIR
	#define NAME "Lanc3"
#endif // IS_AVIR

const double Bias = 0.0;
const double SizeCoeff = 0.3;
const int SrcImageWidth = 1024 * 16;
const int SrcImageHeight = 12;
const int Offs = 32;
const int ImgCapacity = SrcImageWidth * SrcImageHeight;
CFixedBuffer< float > SrcImg( ImgCapacity );
avir :: CImageResizerParamsDef ResizeParams;
double p1g;

class CStats
{
public:
	CSyncObject StateSync;

	double avgd; // Frequency response.
	double avgd2; // Dynamic range (obtained via two-way resizing).
	double peakd; // Peak error (obtained via two-way resizing).
	int avgc;

	CStats()
	{
		reset();
	}

	void reset()
	{
		avgd = 0.0;
		avgd2 = 0.0;
		peakd = 0.0;
		avgc = 0;
	}
};

inline double calcRMS( const float* p, const int l )
{
	double s = 0.0;
	int i;

	for( i = 0; i < l; i++ )
	{
		s += sqr( p[ i ] - Bias );
	}

	return( sqrt( s / l ));
}

inline double calcRMSDiff( const float* const p1, const float* const p2,
	const double p2g, const int l, double& peakd )
{
	peakd = 0.0;
	double s = 0.0;
	int i;

	for( i = 0; i < l; i++ )
	{
		const double d = ( p1[ i ] - Bias ) * p1g - ( p2[ i ] - Bias ) * p2g;
		s += sqr( d );
		peakd = max( peakd, fabs( d ));
	}

	return( sqrt( s / l ));
}

class CResampThread : public CWorkerThread
{
public:
	CStats* Stats; ///< Pointer to summary statistics object.
		///<
	double k; ///< Resize factor.
		///<
	int BitDepth; ///< Resize bit depth.
		///<

protected:
	virtual ecode performWork()
	{
		#if IS_AVIR
			avir :: CImageResizer<> ImageResizer( BitDepth, 0, ResizeParams );
		#else // IS_AVIR
			avir :: CLancIR ImageResizer;
		#endif // IS_AVIR

		const int DstImageWidth = (int) ceil( SrcImageWidth / k );
		const int DstImageHeight = (int) ceil( SrcImageHeight / k );

		CFixedBuffer< float > DstImg( DstImageWidth * DstImageHeight );

		#if IS_AVIR
			ImageResizer.resizeImage( &SrcImg[ 0 ], SrcImageWidth,
				SrcImageHeight, 0, &DstImg[ 0 ], DstImageWidth,
				DstImageHeight, 1, -k );
		#else // IS_AVIR
			ImageResizer.resizeImage( &SrcImg[ 0 ], SrcImageWidth,
				SrcImageHeight, 0, &DstImg[ 0 ], DstImageWidth,
				DstImageHeight, 1, -k, -k, 0.0, 0.0 );
		#endif // IS_AVIR

		CFixedBuffer< float > DstImg2( SrcImageWidth * SrcImageHeight );

		#if IS_AVIR
			ImageResizer.resizeImage( &DstImg[ 0 ], DstImageWidth,
				DstImageHeight, 0, &DstImg2[ 0 ], SrcImageWidth,
				SrcImageHeight, 1, -1.0 / k );
		#else // IS_AVIR
			ImageResizer.resizeImage( &DstImg[ 0 ], DstImageWidth,
				DstImageHeight, 0, &DstImg2[ 0 ], SrcImageWidth,
				SrcImageHeight, 1, -1.0 / k, -1.0 / k, 0.0, 0.0 );
		#endif // IS_AVIR

		const double r = calcRMS( &DstImg[ Offs ], DstImageWidth - Offs * 2 );

		const double p2g = 1.0 / calcRMS( &DstImg2[ Offs ],
			SrcImageWidth - Offs * 2 );

		double peakd = 0.0;
		const double r2 = calcRMSDiff( &SrcImg[ Offs ], &DstImg2[ Offs ],
			p2g, SrcImageWidth - Offs * 2, peakd );

		VOXSYNC( Stats -> StateSync );

		Stats -> avgd += r * r;
		Stats -> avgd2 += r2 * r2;
		Stats -> peakd = max( Stats -> peakd, peakd );
		Stats -> avgc++;

		VOXRET;
	}
};

int main()
{
	int i;

	CWorkerThreadPool Threads;

	for( i = 0; i < CSystem :: getProcessorCount(); i++ )
	{
		Threads.add( new CResampThread() );
	}

	const double minf = 0.01;

	#if IS_UPS
		const double maxf = 0.99;
	#else // IS_UPS
		const double maxf = 0.99 * SizeCoeff;
	#endif // IS_UPS

	const int c = 128;
	int k;

	printf( "\t%s %s\t%s %s\t%s %s\n",
		NAME, " FR", NAME, " DR", NAME, " PE" );

	for( k = 0; k < c; k++ )
	{
		const double th = M_PI * exp( log( minf ) +
			log( maxf / minf ) * k / ( c - 1 ));

		// Prepare image with sinewave content that corresponds to the
		// "th" circular frequency.

		int j;

		for( j = 0; j < SrcImageHeight; j++ )
		{
			double s = 0.0;

			for( i = 0; i < SrcImageWidth; i++ )
			{
				SrcImg[ j * SrcImageWidth + i ] = (float) cos( i * th );
				s += SrcImg[ j * SrcImageWidth + i ];
			}

			s /= SrcImageWidth;
			double s2 = 0.0;

			// Perform de-biasing.

			for( i = 0; i < SrcImageWidth; i++ )
			{
				SrcImg[ j * SrcImageWidth + i ] -= s;
				s2 += sqr( SrcImg[ j * SrcImageWidth + i ]);
			}

			// Perform power normalization, and add a constant Bias value.

			s2 = 1.0 / sqrt( s2 / SrcImageWidth );

			for( i = 0; i < SrcImageWidth; i++ )
			{
				SrcImg[ j * SrcImageWidth + i ] *= s2;
				SrcImg[ j * SrcImageWidth + i ] += Bias;
			}
		}

		p1g = 1.0 / calcRMS( &SrcImg[ Offs ], SrcImageWidth - Offs * 2 );

		// Perform multi-threaded resizing and statistics accumulation over
		// a logarithmically-spaced resizing "k" factors.

		CStats Stats;
		double k = 1.0;

		while( k > SizeCoeff )
		{
			CResampThread* rs;
			VOXERRSKIP( Threads.getIdleThread( rs ));

			rs -> Stats = &Stats;

			#if IS_UPS
				rs -> k = k;
			#else // IS_UPS
				rs -> k = 1.0 / k;
			#endif // IS_UPS

			rs -> BitDepth = 16;
			rs -> start();

			k *= 0.95;
		}

		VOXERRSKIP( Threads.waitAllForFinish() );

		Stats.avgd = 10.0 * log( Stats.avgd / Stats.avgc ) / log( 10.0 );
		Stats.avgd2 = 10.0 * log( Stats.avgd2 / Stats.avgc ) / log( 10.0 );
		Stats.peakd = 20.0 * log( Stats.peakd ) / log( 10.0 );

		printf( "%.6f\t%.6f\t%.6f\t%.6f\n", th / M_PI,
			Stats.avgd, Stats.avgd2, Stats.peakd );
	}
}
