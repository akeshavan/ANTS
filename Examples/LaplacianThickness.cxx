

#include "antsUtilities.h"
#include <algorithm>

#include "itkDanielssonDistanceMapImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkFastMarchingUpwindGradientImageFilter.h"
#include "itkGradientRecursiveGaussianImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkLaplacianRecursiveGaussianImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkVectorCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkVectorLinearInterpolateImageFunction.h"
#include "itkWarpImageFilter.h"
#include "vnl/algo/vnl_determinant.h"

#include "ReadWriteImage.h"

namespace ants
{

template <class TField, class TImage>
typename TImage::Pointer
GetVectorComponent(typename TField::Pointer field, unsigned int index)
{
  // Initialize the Moving to the displacement field
  typedef TField FieldType;
  typedef TImage ImageType;

  typename ImageType::Pointer sfield = ImageType::New();
  sfield->SetSpacing( field->GetSpacing() );
  sfield->SetOrigin( field->GetOrigin() );
  sfield->SetDirection( field->GetDirection() );
  sfield->SetLargestPossibleRegion(field->GetLargestPossibleRegion() );
  sfield->SetRequestedRegion(field->GetRequestedRegion() );
  sfield->SetBufferedRegion( field->GetBufferedRegion() );
  sfield->Allocate();

  typedef itk::ImageRegionIteratorWithIndex<TField> Iterator;
  Iterator vfIter( field,  field->GetLargestPossibleRegion() );
  for( vfIter.GoToBegin(); !vfIter.IsAtEnd(); ++vfIter )
    {
    typename TField::PixelType v1 = vfIter.Get();
    sfield->SetPixel(vfIter.GetIndex(), v1[index]);
    }

  return sfield;

}

template <class TImage>
typename TImage::Pointer
SmoothImage(typename TImage::Pointer image, float sig)
{
// find min value
  typedef itk::ImageRegionIteratorWithIndex<TImage> Iterator;
  Iterator vfIter(image, image->GetLargestPossibleRegion() );
  for( vfIter.GoToBegin(); !vfIter.IsAtEnd(); ++vfIter )
    {
    typename TImage::PixelType v1 = vfIter.Get();
    if( vnl_math_isnan(v1) )
      {
      vfIter.Set(0);
      }
    }
  typedef itk::DiscreteGaussianImageFilter<TImage, TImage> dgf;
  typename dgf::Pointer filter = dgf::New();
  filter->SetVariance(sig);
  filter->SetUseImageSpacingOn();
  filter->SetMaximumError(.01f);
  filter->SetInput(image);
  filter->Update();
  typename TImage::Pointer out = filter->GetOutput();

  return out;

}

template <class TImage>
void
SmoothDeformation(typename TImage::Pointer vectorimage, float sig)
{

  typedef itk::Vector<float, 3> VectorType;
  typedef itk::Image<float, 3>  ImageType;
  typename ImageType::Pointer subimgx = GetVectorComponent<TImage, ImageType>(vectorimage, 0);
  subimgx = SmoothImage<ImageType>(subimgx, sig);
  typename ImageType::Pointer subimgy = GetVectorComponent<TImage, ImageType>(vectorimage, 1);
  subimgy = SmoothImage<ImageType>(subimgy, sig);
  typename ImageType::Pointer subimgz = GetVectorComponent<TImage, ImageType>(vectorimage, 2);
  subimgz = SmoothImage<ImageType>(subimgz, sig);

  typedef itk::ImageRegionIteratorWithIndex<TImage> IteratorType;
  IteratorType Iterator( vectorimage, vectorimage->GetLargestPossibleRegion().GetSize() );
  Iterator.GoToBegin();
  while(  !Iterator.IsAtEnd()  )
    {
    VectorType vec;
    vec[0] = subimgx->GetPixel(Iterator.GetIndex() );
    vec[1] = subimgy->GetPixel(Iterator.GetIndex() );
    vec[2] = subimgz->GetPixel(Iterator.GetIndex() );
    Iterator.Set(vec);
    ++Iterator;
    }

  return;

}

template <class TImage>
typename TImage::Pointer
LabelSurface(typename TImage::PixelType foreground,
             typename TImage::PixelType newval, typename TImage::Pointer input, float distthresh )
{
  antscout << " Label Surf " << std::endl;
  typedef TImage ImageType;
  enum { ImageDimension = ImageType::ImageDimension };
  typename   ImageType::Pointer     Image = ImageType::New();
  Image->SetLargestPossibleRegion(input->GetLargestPossibleRegion()  );
  Image->SetBufferedRegion(input->GetLargestPossibleRegion() );
  Image->Allocate();
  Image->SetSpacing(input->GetSpacing() );
  Image->SetOrigin(input->GetOrigin() );
  typedef itk::NeighborhoodIterator<ImageType> iteratorType;

  typename iteratorType::RadiusType rad;
  for( int j = 0; j < ImageDimension; j++ )
    {
    rad[j] = (unsigned int)(distthresh + 0.5);
    }
  iteratorType GHood(rad, input, input->GetLargestPossibleRegion() );

  GHood.GoToBegin();

//  antscout << " foreg " << (int) foreground;
  while( !GHood.IsAtEnd() )
    {
    typename TImage::PixelType p = GHood.GetCenterPixel();
    typename TImage::IndexType ind = GHood.GetIndex();
    typename TImage::IndexType ind2;
    if( p == foreground )
      {
      bool atedge = false;
      for( unsigned int i = 0; i < GHood.Size(); i++ )
        {
        ind2 = GHood.GetIndex(i);
        float dist = 0.0;
        for( int j = 0; j < ImageDimension; j++ )
          {
          dist += (float)(ind[j] - ind2[j]) * (float)(ind[j] - ind2[j]);
          }
        dist = sqrt(dist);
        if( GHood.GetPixel(i) != foreground && dist <  distthresh  )
          {
          atedge = true;
          }
        }
      if( atedge && p == foreground )
        {
        Image->SetPixel(ind, newval);
        }
      else
        {
        Image->SetPixel(ind, 0);
        }
      }
    ++GHood;
    }

  return Image;
}

template <class TImage>
typename TImage::Pointer  Morphological( typename TImage::Pointer input, float rad, bool option)
{
  typedef TImage ImageType;
  enum { ImageDimension = TImage::ImageDimension };
  typedef typename TImage::PixelType PixelType;

  if( !option )
    {
    antscout << " eroding the image " << std::endl;
    }
  else
    {
    antscout << " dilating the image " << std::endl;
    }
  typedef itk::BinaryBallStructuringElement<
    PixelType,
    ImageDimension>             StructuringElementType;

  // Software Guide : BeginCodeSnippet
  typedef itk::BinaryErodeImageFilter<
    TImage,
    TImage,
    StructuringElementType>  ErodeFilterType;

  typedef itk::BinaryDilateImageFilter<
    TImage,
    TImage,
    StructuringElementType>  DilateFilterType;

  typename ErodeFilterType::Pointer  binaryErode  = ErodeFilterType::New();
  typename DilateFilterType::Pointer binaryDilate = DilateFilterType::New();

  StructuringElementType structuringElement;

  structuringElement.SetRadius( (unsigned long) rad );  // 3x3x3 structuring element

  structuringElement.CreateStructuringElement();

  binaryErode->SetKernel(  structuringElement );
  binaryDilate->SetKernel( structuringElement );

  //  It is necessary to define what could be considered objects on the binary
  //  images. This is specified with the methods \code{SetErodeValue()} and
  //  \code{SetDilateValue()}. The value passed to these methods will be
  //  considered the value over which the dilation and erosion rules will apply
  binaryErode->SetErodeValue( 1 );
  binaryDilate->SetDilateValue( 1 );

  typename TImage::Pointer temp;
  if( option )
    {
    binaryDilate->SetInput( input );
    binaryDilate->Update();
    temp = binaryDilate->GetOutput();
    }
  else
    {
    binaryErode->SetInput( input );  // binaryDilate->GetOutput() );
    binaryErode->Update();
    temp = binaryErode->GetOutput();

    typedef itk::ImageRegionIteratorWithIndex<ImageType> ImageIteratorType;
    ImageIteratorType o_iter( temp, temp->GetLargestPossibleRegion() );
    o_iter.GoToBegin();
    while( !o_iter.IsAtEnd() )
      {
      if( o_iter.Get() > 0.5 && input->GetPixel(o_iter.GetIndex() ) > 0.5 )
        {
        o_iter.Set(1);
        }
      else
        {
        o_iter.Set(0);
        }
      ++o_iter;
      }

    }

  return temp;

}

template <class TImage, class TField>
typename TField::Pointer
FMMGrad(typename TImage::Pointer wm, typename TImage::Pointer gm )
{
  typedef TImage ImageType;
  enum { ImageDimension = TImage::ImageDimension };
  typename TField::Pointer sfield = TField::New();
  sfield->SetSpacing( wm->GetSpacing() );
  sfield->SetOrigin( wm->GetOrigin() );
  sfield->SetDirection( wm->GetDirection() );
  sfield->SetLargestPossibleRegion(wm->GetLargestPossibleRegion() );
  sfield->SetRequestedRegion(wm->GetRequestedRegion() );
  sfield->SetBufferedRegion( wm->GetBufferedRegion() );
  sfield->Allocate();
  typename ImageType::Pointer surf = LabelSurface<ImageType>(1, 1, wm, 1.9 );

  typedef itk::FastMarchingUpwindGradientImageFilter<ImageType, ImageType> FloatFMType;
  typename FloatFMType::Pointer marcher = FloatFMType::New();
  typedef typename FloatFMType::NodeType      NodeType;
  typedef typename FloatFMType::NodeContainer NodeContainer;
  // setup alive points
  typename NodeContainer::Pointer alivePoints = NodeContainer::New();
  typename NodeContainer::Pointer targetPoints = NodeContainer::New();
  typename NodeContainer::Pointer trialPoints = NodeContainer::New();
  typedef itk::ImageRegionIteratorWithIndex<TImage> IteratorType;
  IteratorType thIt( wm, wm->GetLargestPossibleRegion().GetSize() );
  thIt.GoToBegin();
  unsigned long bb = 0, cc = 0, dd = 0;
  while(  !thIt.IsAtEnd()  )
    {
    if( thIt.Get() > 0.1 && surf->GetPixel(thIt.GetIndex() ) == 0 )
      {
      NodeType node;
      node.SetValue( 0 );
      node.SetIndex(thIt.GetIndex() );
      alivePoints->InsertElement(bb, node);
      bb++;
      }
    if( gm->GetPixel(thIt.GetIndex() ) == 0 && wm->GetPixel(thIt.GetIndex() ) == 0  )
      {
      NodeType node;
      node.SetValue( 0 );
      node.SetIndex(thIt.GetIndex() );
      targetPoints->InsertElement(cc, node);
      cc++;
      }
    if( surf->GetPixel(thIt.GetIndex() ) == 1 )
      {
      NodeType node;
      node.SetValue( 0 );
      node.SetIndex(thIt.GetIndex() );
      trialPoints->InsertElement(cc, node);
      dd++;
      }
    ++thIt;
    }

  marcher->SetTargetReachedModeToAllTargets();
  marcher->SetAlivePoints( alivePoints );
  marcher->SetTrialPoints( trialPoints );
  marcher->SetTargetPoints( targetPoints );
  marcher->SetInput( gm );
  double stoppingValue = 1000.0;
  marcher->SetStoppingValue( stoppingValue );
  marcher->GenerateGradientImageOn();
  marcher->Update();
  WriteImage<ImageType>(marcher->GetOutput(), "marcher.nii.gz");

  thIt.GoToBegin();
  while(  !thIt.IsAtEnd()  )
    {
    typename TField::PixelType vec;
    for( dd = 0; dd < ImageDimension; dd++ )
      {
      vec[dd] = marcher->GetGradientImage()->GetPixel(thIt.GetIndex() )[dd];
      }
    ++thIt;
    }

  return sfield;
}

template <class TImage, class TField>
typename TField::Pointer
LaplacianGrad(typename TImage::Pointer wm, typename TImage::Pointer gm, float sig, unsigned int numits, float tolerance)
{
  typedef  typename TImage::IndexType IndexType;
  IndexType ind;
  typedef TImage ImageType;
  typedef TField GradientImageType;
  typedef itk::GradientRecursiveGaussianImageFilter<ImageType, GradientImageType>
  GradientImageFilterType;
  typedef typename GradientImageFilterType::Pointer GradientImageFilterPointer;

  typename TField::Pointer sfield = TField::New();
  sfield->SetSpacing( wm->GetSpacing() );
  sfield->SetOrigin( wm->GetOrigin() );
  sfield->SetDirection( wm->GetDirection() );
  sfield->SetLargestPossibleRegion(wm->GetLargestPossibleRegion() );
  sfield->SetRequestedRegion(wm->GetRequestedRegion() );
  sfield->SetBufferedRegion( wm->GetBufferedRegion() );
  sfield->Allocate();

  typename TImage::Pointer laplacian = SmoothImage<TImage>(wm, 1);
  laplacian->FillBuffer(0);
  typedef itk::ImageRegionIteratorWithIndex<TImage> IteratorType;
  IteratorType Iterator( wm, wm->GetLargestPossibleRegion().GetSize() );
  Iterator.GoToBegin();

  // initialize L(wm)=1, L(gm)=0.5, else 0
  while(  !Iterator.IsAtEnd()  )
    {
    ind = Iterator.GetIndex();
    if( wm->GetPixel(ind) >= 0.5 )
      {
      laplacian->SetPixel(ind, 1);
      }
    else if( gm->GetPixel(ind) >= 0.5  && wm->GetPixel(ind) < 0.5 )
      {
      laplacian->SetPixel(ind, 2.);
      }
    else
      {
      laplacian->SetPixel(ind, 2.);
      }
    ++Iterator;
    }

  // smooth and then reset the values
  float        meanvalue = 0, lastmean = 1;
  unsigned int iterations = 0;
  while( fabs(meanvalue - lastmean) > tolerance  && iterations < numits )
    {
    iterations++;
    antscout << "  % " << (float) iterations
    / (float)(numits + 1) << " delta-mean " << fabs(meanvalue - lastmean) <<  std::endl;
    laplacian = SmoothImage<TImage>(laplacian, sqrt(sig) );
    Iterator.GoToBegin();
    unsigned int ct = 0;
    lastmean = meanvalue;
    while(  !Iterator.IsAtEnd()  )
      {
      ind = Iterator.GetIndex();
      if( wm->GetPixel(ind) >= 0.5 )
        {
        laplacian->SetPixel(ind, 1);
        }
      else if( gm->GetPixel(ind) < 0.5  && wm->GetPixel(ind) < 0.5 )
        {
        laplacian->SetPixel(ind, 2.);
        }
      else
        {
        meanvalue += laplacian->GetPixel(ind);  ct++;
        }
      ++Iterator;
      }

    meanvalue /= (float)ct;
    }

  // /  WriteImage<ImageType>(laplacian, "laplacian.hdr");

  GradientImageFilterPointer filter = GradientImageFilterType::New();
  filter->SetInput(  laplacian );
  filter->SetSigma(sig * 0.5);
  filter->Update();
  return filter->GetOutput();

}

template <class TImage, class TField, class TInterp, class TInterp2>
float IntegrateLength( typename TImage::Pointer /* gmsurf */,  typename TImage::Pointer /* thickimage */,
                       typename TImage::IndexType velind,  typename TField::Pointer lapgrad,  float itime,
                       float starttime, float /* finishtime */,
                       bool timedone, float deltaTime, typename TInterp::Pointer vinterp,
                       typename TInterp2::Pointer sinterp, unsigned int /* task */,
                       bool /* propagate */, bool domeasure,   unsigned int m_NumberOfTimePoints,
                       typename TImage::SpacingType spacing, float vecsign,
                       float timesign, float gradsign, unsigned int ct, typename TImage::Pointer wm,
                       typename TImage::Pointer gm,
                       float priorthickval,  typename TImage::Pointer smooththick, bool printprobability,
                       typename TImage::Pointer /* sulci */ )
{
  typedef   TField                                                 TimeVaryingVelocityFieldType;
  typedef typename TField::PixelType                               VectorType;
  typedef itk::ImageRegionIteratorWithIndex<TField>                FieldIterator;
  typedef typename TField::IndexType                               DIndexType;
  typedef typename TField::PointType                               DPointType;
  typedef itk::VectorLinearInterpolateImageFunction<TField, float> DefaultInterpolatorType;

  VectorType zero;
  zero.Fill(0);
  VectorType disp;
  disp.Fill(0);
  ct = 0;
  DPointType pointIn1;
  DPointType pointIn2;
  typename DefaultInterpolatorType::ContinuousIndexType  vcontind;
  DPointType pointIn3;
  enum { ImageDimension = TImage::ImageDimension };
  typedef typename TImage::IndexType IndexType;
  for( unsigned int jj = 0; jj < ImageDimension; jj++ )
    {
    IndexType index;
    index[jj] = velind[jj];
    pointIn1[jj] = velind[jj] * lapgrad->GetSpacing()[jj];
    }
  // if( task == 0 )
  //   {
  //   propagate = false;
  //   }
  // else
  //   {
  //   propagate = true;
  //   }
  itime = starttime;
  timedone = false;
  float totalmag = 0;
  if( domeasure )
    {
    while( !timedone )
      {
      float scale = 1; // *m_DT[timeind]/m_DS[timeind];
      //     antscout << " scale " << scale << std::endl;
      double itimetn1 = itime - timesign * deltaTime * scale;
      double itimetn1h = itime - timesign * deltaTime * 0.5 * scale;
      if( itimetn1h < 0 )
        {
        itimetn1h = 0;
        }
      if( itimetn1h > m_NumberOfTimePoints - 1 )
        {
        itimetn1h = m_NumberOfTimePoints - 1;
        }
      if( itimetn1 < 0 )
        {
        itimetn1 = 0;
        }
      if( itimetn1 > m_NumberOfTimePoints - 1 )
        {
        itimetn1 = m_NumberOfTimePoints - 1;
        }
      // first get current position of particle
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        IndexType index;
        index[jj] = velind[jj];
        pointIn1[jj] = velind[jj] * lapgrad->GetSpacing()[jj];
        }
      //      antscout << " ind " << index  << std::endl;
      // now index the time varying field at that position.
      typename DefaultInterpolatorType::OutputType f1;  f1.Fill(0);
      typename DefaultInterpolatorType::OutputType f2;  f2.Fill(0);
      typename DefaultInterpolatorType::OutputType f3;  f3.Fill(0);
      typename DefaultInterpolatorType::OutputType f4;  f4.Fill(0);
      typename DefaultInterpolatorType::ContinuousIndexType  Y1;
      typename DefaultInterpolatorType::ContinuousIndexType  Y2;
      typename DefaultInterpolatorType::ContinuousIndexType  Y3;
      typename DefaultInterpolatorType::ContinuousIndexType  Y4;
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        pointIn2[jj] = disp[jj] + pointIn1[jj];
        vcontind[jj] = pointIn2[jj] / lapgrad->GetSpacing()[jj];
        Y1[jj] = vcontind[jj];
        Y2[jj] = vcontind[jj];
        Y3[jj] = vcontind[jj];
        Y4[jj] = vcontind[jj];
        }
      // Y1[ImageDimension]=itimetn1;
      // Y2[ImageDimension]=itimetn1h;
      // Y3[ImageDimension]=itimetn1h;
      //      Y4[ImageDimension]=itime;

      f1 = vinterp->EvaluateAtContinuousIndex( Y1 );
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        Y2[jj] += f1[jj] * deltaTime * 0.5;
        }
      bool isinside = true;
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        if( Y2[jj] < 1 || Y2[jj] > lapgrad->GetLargestPossibleRegion().GetSize()[jj] - 2 )
          {
          isinside = false;
          }
        }
      if( isinside )
        {
        f2 = vinterp->EvaluateAtContinuousIndex( Y2 );
        }
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        Y3[jj] += f2[jj] * deltaTime * 0.5;
        }
      isinside = true;
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        if( Y3[jj] < 1 || Y3[jj] > lapgrad->GetLargestPossibleRegion().GetSize()[jj] - 2 )
          {
          isinside = false;
          }
        }
      if( isinside )
        {
        f3 = vinterp->EvaluateAtContinuousIndex( Y3 );
        }
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        Y4[jj] += f3[jj] * deltaTime;
        }
      isinside = true;
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        if( Y4[jj] < 1 || Y4[jj] > lapgrad->GetLargestPossibleRegion().GetSize()[jj] - 2 )
          {
          isinside = false;
          }
        }
      if( isinside )
        {
        f4 = vinterp->EvaluateAtContinuousIndex( Y4 );
        }
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        pointIn3[jj] = pointIn2[jj] + gradsign * vecsign * deltaTime / 6.0
          * ( f1[jj] + 2.0 * f2[jj] + 2.0 * f3[jj] + f4[jj] );
        }

      VectorType out;
      float      mag = 0, dmag = 0;
      for( unsigned int jj = 0; jj < ImageDimension; jj++ )
        {
        out[jj] = pointIn3[jj] - pointIn1[jj];
        mag += (pointIn3[jj] - pointIn2[jj]) * (pointIn3[jj] - pointIn2[jj]);
        dmag += (pointIn3[jj] - pointIn1[jj]) * (pointIn3[jj] - pointIn1[jj]);
        disp[jj] = out[jj];
        }
      dmag = sqrt(dmag);
      totalmag += sqrt(mag);

      ct++;
      //      if (!propagate) //thislength=dmag;//
//         thislength += totalmag;
      itime = itime + deltaTime * timesign;
      IndexType myind;
      for( unsigned int qq = 0; qq <  ImageDimension; qq++ )
        {
        myind[qq] = (unsigned long)(pointIn3[qq] / spacing[qq] + 0.5);
        }

      if( (gm->GetPixel(myind) < 0.5 && wm->GetPixel(myind) < 0.5) ||
          (wm->GetPixel(myind) >= 0.5 && gm->GetPixel(myind) < 0.5) ||
          mag < 1.e-1 * deltaTime )
        {
        timedone = true;
        }
      if( gm->GetPixel(myind) < 0.5 )
        {
        timedone = true;
        }
      if( ct >  2.0 / deltaTime )
        {
        timedone = true;
        }
      if( totalmag >  priorthickval )
        {
        timedone = true;
        }
      if( smooththick )
        {
        if( (totalmag - smooththick->GetPixel(velind) ) > 1 )
          {
          timedone = true;
          }
        }

      if( printprobability )
        {
        antscout << " ind " << Y1 << " prob " << sinterp->EvaluateAtContinuousIndex(Y1) << " t " << itime << std::endl;
        }

      }
    }

  return totalmag;

}

template <unsigned int ImageDimension>
int LaplacianThickness(int argc, char *argv[])
{

  float        gradstep = -50.0; // atof(argv[3])*(-1.0);
  unsigned int nsmooth = 2;
  float        smoothparam = 1;
  float        priorthickval = 500;
  double       dT = 0.01;
  std::string  wfn = std::string(argv[1]);
  std::string  gfn = std::string(argv[2]);
  int          argct = 3;
  std::string  outname = std::string(argv[argct]); argct++;

  if( argc > argct )
    {
    smoothparam = atof(argv[argct]);
    }
  argct++;
  if( argc > argct )
    {
    priorthickval = atof(argv[argct]);
    }
  argct++;
  if( argc > argct )
    {
    dT = atof(argv[argct]);
    }
  argct++;
  float dosulc = 0;
  if( argc >  argct )
    {
    dosulc = atof(argv[argct]);
    }
  argct++;
  float tolerance = 0.001;
  if( argc >  argct )
    {
    tolerance = atof(argv[argct]);
    }
  argct++;
  antscout << " using tolerance " << tolerance << std::endl;
  typedef float                                      PixelType;
  typedef itk::Vector<float, ImageDimension>         VectorType;
  typedef itk::Image<VectorType, ImageDimension>     DisplacementFieldType;
  typedef itk::Image<PixelType, ImageDimension>      ImageType;
  typedef itk::ImageFileReader<ImageType>            readertype;
  typedef itk::ImageFileWriter<ImageType>            writertype;
  typedef typename  ImageType::IndexType             IndexType;
  typedef typename  ImageType::SizeType              SizeType;
  typedef typename  ImageType::SpacingType           SpacingType;
  typedef itk::Image<VectorType, ImageDimension + 1> tvt;

  //  typename tvt::Pointer gWarp;
  // ReadImage<tvt>( gWarp, ifn.c_str() );

  typename ImageType::Pointer thickimage;
  ReadImage<ImageType>(thickimage, wfn.c_str() );
  thickimage->FillBuffer(0);
  typename ImageType::Pointer thickimage2;
  ReadImage<ImageType>(thickimage2, wfn.c_str() );
  thickimage2->FillBuffer(0);
  typename ImageType::Pointer wm;
  ReadImage<ImageType>(wm, wfn.c_str() );
  typename ImageType::Pointer gm;
  ReadImage<ImageType>(gm, gfn.c_str() );
  SpacingType spacing = wm->GetSpacing();
  typedef itk::ImageRegionIteratorWithIndex<ImageType> IteratorType;
  IteratorType Iterator( wm, wm->GetLargestPossibleRegion().GetSize() );
  typename ImageType::Pointer wmb = BinaryThreshold<ImageType>(0.5, 1.e9, 1, wm);
  typename DisplacementFieldType::Pointer lapgrad = NULL;
  typename DisplacementFieldType::Pointer lapgrad2 = NULL;
  typename ImageType::Pointer gmb = BinaryThreshold<ImageType>(0.5, 1.e9, 1, gm);

/** get sulcal priors */
  typename ImageType::Pointer sulci = NULL;
  if( dosulc > 0 )
    {
    antscout << "  using sulcal prior " << std::endl;
    typedef itk::DanielssonDistanceMapImageFilter<ImageType, ImageType> FilterType;
    typename  FilterType::Pointer distmap = FilterType::New();
    distmap->InputIsBinaryOn();
    distmap->SetUseImageSpacing(true);
    distmap->SetInput(wmb);
    distmap->Update();
    typename ImageType::Pointer distwm = distmap->GetOutput();

    typedef itk::LaplacianRecursiveGaussianImageFilter<ImageType, ImageType> dgf;
    typename dgf::Pointer lfilter = dgf::New();
    lfilter->SetSigma(smoothparam);
    lfilter->SetInput(distwm);
    lfilter->Update();
    typename ImageType::Pointer image2 = lfilter->GetOutput();
    typedef itk::RescaleIntensityImageFilter<ImageType, ImageType> RescaleFilterType;
    typename RescaleFilterType::Pointer rescaler = RescaleFilterType::New();
    rescaler->SetOutputMinimum(   0 );
    rescaler->SetOutputMaximum( 1 );
    rescaler->SetInput( image2 );
    rescaler->Update();
    sulci =  rescaler->GetOutput();
    WriteImage<ImageType>(sulci, "sulci.nii");

    Iterator.GoToBegin();
    while(  !Iterator.IsAtEnd()  )
      {
//    antscout << " a good value for use sulcus prior is 0.002  -- in a function :
//  1/(1.+exp(-0.1*(sulcprob-0.275)/use-sulcus-prior)) " << std::endl;
//
      float gmprob = gm->GetPixel(Iterator.GetIndex() );
      if( gmprob == 0 )
        {
        gmprob = 0.05;
        }
      float sprob = sulci->GetPixel(Iterator.GetIndex() );
      sprob = 1 / (1. + exp(-0.1 * (sprob - 0.5) / dosulc) );
      sulci->SetPixel(Iterator.GetIndex(), sprob );
//    if (gmprob > 0) antscout << " gmp " << gmprob << std::endl;
      ++Iterator;
      }

    antscout << " modified gm prior by sulcus prior " << std::endl;
    WriteImage<ImageType>(sulci, "sulcigm.nii");

    typedef itk::GradientRecursiveGaussianImageFilter<ImageType, DisplacementFieldType>
    GradientImageFilterType;
    typedef typename GradientImageFilterType::Pointer GradientImageFilterPointer;
    GradientImageFilterPointer filter = GradientImageFilterType::New();
    filter->SetInput(  distwm );
    filter->SetSigma(smoothparam);
    filter->Update();
    lapgrad2 = filter->GetOutput();

//      return 0;
/** sulc priors done */
    }

  lapgrad = LaplacianGrad<ImageType, DisplacementFieldType>(wmb, gmb, smoothparam, 500, tolerance);
  //  lapgrad=FMMGrad<ImageType,DisplacementFieldType>(wmb,gmb);

  //  LabelSurface(typename TImage::PixelType foreground,
  //       typename TImage::PixelType newval, typename TImage::Pointer input, float distthresh )
  float distthresh = 1.9;

  typename ImageType::Pointer wmgrow = Morphological<ImageType>(wmb, 1, true);
  typename ImageType::Pointer surf = LabelSurface<ImageType>(1, 1, wmgrow, distthresh);
  typename ImageType::Pointer gmsurf = LabelSurface<ImageType>(1, 1, gmb, distthresh);
  // now integrate
  //

  double timezero = 0; // 1
  double timeone = 1;  // (s[ImageDimension]-1-timezero);

  //  unsigned int m_NumberOfTimePoints = s[ImageDimension];

  float starttime = timezero; // timezero;
  float finishtime = timeone; // s[ImageDimension]-1;//timeone;
  // antscout << " MUCKING WITH START FINISH TIME " <<  finishtime <<  std::endl;

  typename DisplacementFieldType::IndexType velind;
  typename ImageType::Pointer smooththick = NULL;
  float timesign = 1.0;
  if( starttime  >  finishtime )
    {
    timesign = -1.0;
    }
  unsigned int m_NumberOfTimePoints = 2;
  typedef   DisplacementFieldType                                                        TimeVaryingVelocityFieldType;
  typedef itk::ImageRegionIteratorWithIndex<DisplacementFieldType>                       FieldIterator;
  typedef typename DisplacementFieldType::IndexType                                      DIndexType;
  typedef typename DisplacementFieldType::PointType                                      DPointType;
  typedef typename TimeVaryingVelocityFieldType::IndexType                               VIndexType;
  typedef typename TimeVaryingVelocityFieldType::PointType                               VPointType;
  typedef itk::VectorLinearInterpolateImageFunction<TimeVaryingVelocityFieldType, float> DefaultInterpolatorType;
  typedef itk::VectorLinearInterpolateImageFunction<DisplacementFieldType, float>        DefaultInterpolatorType2;
  typename DefaultInterpolatorType::Pointer vinterp =  DefaultInterpolatorType::New();
  typedef itk::LinearInterpolateImageFunction<ImageType, float> ScalarInterpolatorType;
  typename ScalarInterpolatorType::Pointer sinterp =  ScalarInterpolatorType::New();
  sinterp->SetInputImage(gm);
  if( sulci )
    {
    sinterp->SetInputImage(sulci);
    }
  VectorType zero;
  zero.Fill(0);

  DPointType pointIn1;
  DPointType pointIn2;
  typename DefaultInterpolatorType::ContinuousIndexType  vcontind;
  DPointType pointIn3;

  typedef itk::ImageRegionIteratorWithIndex<DisplacementFieldType> VIteratorType;
  VIteratorType VIterator( lapgrad, lapgrad->GetLargestPossibleRegion().GetSize() );
  VIterator.GoToBegin();
  while(  !VIterator.IsAtEnd()  )
    {
    VectorType vec = VIterator.Get();
    float      mag = 0;
    for( unsigned int qq = 0; qq < ImageDimension; qq++ )
      {
      mag += vec[qq] * vec[qq];
      }
    mag = sqrt(mag);
    if( mag > 0 )
      {
      vec = vec / mag;
      }
    VIterator.Set(vec * gradstep);
    if( lapgrad2 )
      {
      vec = lapgrad2->GetPixel(VIterator.GetIndex() );
      mag = 0;
      for( unsigned int qq = 0; qq < ImageDimension; qq++ )
        {
        mag += vec[qq] * vec[qq];
        }
      mag = sqrt(mag);
      if( mag > 0 )
        {
        vec = vec / mag;
        }
      lapgrad2->SetPixel(VIterator.GetIndex(), vec * gradstep);
      }
    ++VIterator;
    }

  bool propagate = false;
  for( unsigned int smoothit = 0; smoothit < nsmooth; smoothit++ )
    {
    antscout << " smoothit " << smoothit << std::endl;
    Iterator.GoToBegin();
    unsigned int cter = 0;
    while(  !Iterator.IsAtEnd()  )
      {
      velind = Iterator.GetIndex();
      //      float thislength=0;
      for( unsigned int task = 0; task < 1; task++ )
        {
        float itime = starttime;

        unsigned long ct = 0;
        bool          timedone = false;

        VectorType disp;
        disp.Fill(0.0);
        double deltaTime = dT, vecsign = 1.0;
        bool   domeasure = false;
        float  gradsign = 1.0;
        bool   printprobability = false;
//    antscout << " wmb " << wmb->GetPixel(velind) << " gm " << gm->GetPixel(velind) << std::endl;
//    if (surf->GetPixel(velind) != 0) printprobability=true;
        if( gm->GetPixel(velind) > 0.25 ) // && wmb->GetPixel(velind) < 1 )
          {
          cter++;
          domeasure = true;
          }
        vinterp->SetInputImage(lapgrad);
        gradsign = -1.0; vecsign = -1.0;
        float len1 = IntegrateLength<ImageType, DisplacementFieldType, DefaultInterpolatorType, ScalarInterpolatorType>
            (gmsurf, thickimage, velind, lapgrad,  itime, starttime, finishtime,  timedone,  deltaTime,  vinterp,
            sinterp, task, propagate, domeasure, m_NumberOfTimePoints, spacing, vecsign, gradsign, timesign, ct, wm, gm,
            priorthickval, smooththick, printprobability,
            sulci );

        gradsign = 1.0;  vecsign = 1;
        float len2 = IntegrateLength<ImageType, DisplacementFieldType, DefaultInterpolatorType, ScalarInterpolatorType>
            (gmsurf, thickimage, velind, lapgrad,  itime, starttime, finishtime,  timedone,  deltaTime,  vinterp,
            sinterp, task, propagate, domeasure, m_NumberOfTimePoints, spacing, vecsign, gradsign, timesign, ct, wm, gm,
            priorthickval - len1, smooththick, printprobability,
            sulci );

        float len3 = 1.e9, len4 = 1.e9;
        if( lapgrad2 )
          {
          vinterp->SetInputImage(lapgrad2);
          gradsign = -1.0; vecsign = -1.0;
          len3 = IntegrateLength<ImageType, DisplacementFieldType, DefaultInterpolatorType, ScalarInterpolatorType>
              (gmsurf, thickimage, velind, lapgrad2,  itime, starttime, finishtime,  timedone,  deltaTime,  vinterp,
              sinterp, task, propagate, domeasure, m_NumberOfTimePoints, spacing, vecsign, gradsign, timesign, ct, wm,
              gm,
              priorthickval, smooththick, printprobability,
              sulci );

          gradsign = 1.0;  vecsign = 1;
          len4 = IntegrateLength<ImageType, DisplacementFieldType, DefaultInterpolatorType, ScalarInterpolatorType>
              (gmsurf, thickimage, velind, lapgrad2,  itime, starttime, finishtime,  timedone,  deltaTime,  vinterp,
              sinterp, task, propagate, domeasure, m_NumberOfTimePoints, spacing, vecsign, gradsign, timesign, ct, wm,
              gm,
              priorthickval - len3, smooththick, printprobability,
              sulci );
          }
        float totalength = len1 + len2;
//    if (totalength > 5 && totalength <  8) antscout<< " t1 " << len3+len4 << " t2 " << len1+len2 << std::endl;
        if( len3 + len4 < totalength )
          {
          totalength = len3 + len4;
          }

        if( smoothit == 0 )
          {
          if( thickimage2->GetPixel(velind) == 0  )
            {
            thickimage2->SetPixel(velind, totalength);
            }
          else if( (totalength) > 0 &&  thickimage2->GetPixel(velind) < (totalength) )
            {
            thickimage2->SetPixel(velind, totalength);
            }
          }
        if( smoothit > 0 && smooththick )
          {
          thickimage2->SetPixel(velind, (totalength) * 0.5 + smooththick->GetPixel(velind) * 0.5 );
          }

        if( domeasure && (totalength) > 0 && cter % 10000 == 0 )
          {
          antscout << " len1 " << len1 << " len2 " << len2 << " ind " << velind << std::endl;
          }

        }
      ++Iterator;
      }

    smooththick = SmoothImage<ImageType>(thickimage2, 1.0);

// set non-gm voxels to zero
    IteratorType gIterator( gm, gm->GetLargestPossibleRegion().GetSize() );
    gIterator.GoToBegin();
    while(  !gIterator.IsAtEnd()  )
      {
      if( gm->GetPixel(gIterator.GetIndex() ) < 0.25 )
        {
        thickimage2->SetPixel(gIterator.GetIndex(), 0);
        }
      ++gIterator;
      }

    antscout << " writing " << outname << std::endl;
    WriteImage<ImageType>(thickimage2, outname.c_str() );

    }
//  WriteImage<ImageType>(thickimage,"turd.hdr");

  return 0;

}

// entry point for the library; parameter 'args' is equivalent to 'argv' in (argc,argv) of commandline parameters to
// 'main()'
int LaplacianThickness( std::vector<std::string> args, std::ostream* out_stream = NULL )
{
  // put the arguments coming in as 'args' into standard (argc,argv) format;
  // 'args' doesn't have the command name as first, argument, so add it manually;
  // 'args' may have adjacent arguments concatenated into one argument,
  // which the parser should handle
  args.insert( args.begin(), "LaplacianThickness" );

  std::remove( args.begin(), args.end(), std::string( "" ) );
  int     argc = args.size();
  char* * argv = new char *[args.size() + 1];
  for( unsigned int i = 0; i < args.size(); ++i )
    {
    // allocate space for the string plus a null character
    argv[i] = new char[args[i].length() + 1];
    std::strncpy( argv[i], args[i].c_str(), args[i].length() );
    // place the null character in the end
    argv[i][args[i].length()] = '\0';
    }
  argv[argc] = 0;
  // class to automatically cleanup argv upon destruction
  class Cleanup_argv
  {
public:
    Cleanup_argv( char* * argv_, int argc_plus_one_ ) : argv( argv_ ), argc_plus_one( argc_plus_one_ )
    {
    }

    ~Cleanup_argv()
    {
      for( unsigned int i = 0; i < argc_plus_one; ++i )
        {
        delete[] argv[i];
        }
      delete[] argv;
    }

private:
    char* *      argv;
    unsigned int argc_plus_one;
  };
  Cleanup_argv cleanup_argv( argv, argc + 1 );

  antscout->set_stream( out_stream );

  if( argc < 4 )
    {
    antscout << "Usage:   " << argv[0]
             <<
    " WM.nii GM.nii   Out.nii  {smoothparam=3} {priorthickval=5}  {dT=0.01}  use-sulcus-prior optional-laplacian-tolerance=0.001"
             << std::endl;
    antscout
    <<
    " a good value for use sulcus prior is 0.15 -- in a function :  1/(1.+exp(-0.1*(laplacian-img-value-sulcprob)/0.01)) "
    << std::endl;
    return 1;
    }

  std::string ifn = std::string(argv[1]);
  //  antscout << " image " << ifn << std::endl;
  // Get the image dimension
  itk::ImageIOBase::Pointer imageIO =
    itk::ImageIOFactory::CreateImageIO(ifn.c_str(), itk::ImageIOFactory::ReadMode);
  imageIO->SetFileName(ifn.c_str() );
  imageIO->ReadImageInformation();
  unsigned int dim =  imageIO->GetNumberOfDimensions();

  //   antscout << " dim " << dim << std::endl;
  switch( dim )
    {
    case 2:
      LaplacianThickness<2>(argc, argv);
      break;
    case 3:
      LaplacianThickness<3>(argc, argv);
      break;
    default:
      antscout << "Unsupported dimension" << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

} // namespace ants
