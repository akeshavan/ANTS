#ifndef __itkantsRegistrationHelper_hxx
#define __itkantsRegistrationHelper_hxx

namespace ants
{

/** \class antsRegistrationCommandIterationUpdate
 *  \brief change parameters between iterations of registration
 */
template <class TFilter>
class antsRegistrationCommandIterationUpdate : public itk::Command
{
public:
  typedef antsRegistrationCommandIterationUpdate Self;
  typedef itk::Command                           Superclass;
  typedef itk::SmartPointer<Self>                Pointer;
  itkNewMacro( Self );
protected:
  antsRegistrationCommandIterationUpdate()
  {
    std::cout << "ECHO HACK: INIT NEW CLASS" << std::endl;
    m_clock.Start();
    m_clock.Stop();
    const itk::RealTimeClock::TimeStampType now=m_clock.GetTotal ();
    this->m_lastTotalTime=now;
    m_clock.Start();
    this->m_LogStream = &::ants::antscout;
  }

public:

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *) caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event )
  {
    TFilter const * const filter = dynamic_cast<const TFilter *>( object );

    if( typeid( event ) == typeid( itk::InitializeEvent ) )
      {
      const unsigned int currentLevel = filter->GetCurrentLevel();

      typename TFilter::ShrinkFactorsArrayType shrinkFactors = filter->GetShrinkFactorsPerLevel();
      typename TFilter::SmoothingSigmasArrayType smoothingSigmas = filter->GetSmoothingSigmasPerLevel();
      typename TFilter::TransformParametersAdaptorsContainerType adaptors =
        filter->GetTransformParametersAdaptorsPerLevel();

      m_clock.Stop();
      const itk::RealTimeClock::TimeStampType now=m_clock.GetTotal ();
      this->Logger() << "  Current level = " << currentLevel + 1 << " of " << this->m_NumberOfIterations.size()
                     << std::endl;
      this->Logger() << "    number of iterations = " << this->m_NumberOfIterations[currentLevel] << std::endl;
      this->Logger() << "    shrink factors = " << shrinkFactors[currentLevel] << std::endl;
      this->Logger() << "    smoothing sigmas = " << smoothingSigmas[currentLevel] << std::endl;
      this->Logger() << "    required fixed parameters = " << adaptors[currentLevel]->GetRequiredFixedParameters() << std::endl;
      //this->Logger() << "\n  LEVEL_TIME_INDEX: " << now << " SINCE_LAST: " << (now-this->m_lastTotalTime) << std::endl;
      this->m_lastTotalTime=now;
      m_clock.Start();

      typedef itk::GradientDescentOptimizerv4 GradientDescentOptimizerType;
      GradientDescentOptimizerType * optimizer = reinterpret_cast<GradientDescentOptimizerType *>(
          const_cast<typename TFilter::OptimizerType *>( const_cast<TFilter *>( filter )->GetOptimizer() ) );

      optimizer->SetNumberOfIterations( this->m_NumberOfIterations[currentLevel] );
      }
    else if( typeid( event ) == typeid( itk::IterationEvent ) )
      {
      const unsigned int lCurrentIteration=filter->GetCurrentIteration();
      if ( lCurrentIteration  == 1 )
	{
	//Print header line one time
	this->Logger() << "XDIAGNOSTIC,Iteration,metricValue,convergenceValue,ITERATION_TIME_INDEX,SINCE_LAST" << std::endl;
	}

      m_clock.Stop();
      const itk::RealTimeClock::TimeStampType now=m_clock.GetTotal ();
      this->Logger() << "DIAGNOSTIC, "
	             << lCurrentIteration << ", "
                     << filter->GetCurrentMetricValue() << ", "
                     << filter->GetCurrentConvergenceValue() << ", "
                     << now << ", "
		     << (now-this->m_lastTotalTime) << ", "
                     << std::endl;
      this->m_lastTotalTime=now;
      m_clock.Start();
      }
  }

  void SetNumberOfIterations( const std::vector<unsigned int> & iterations )
  {
    this->m_NumberOfIterations = iterations;
  }

  void SetLogStream(std::ostream & logStream)
  {
    this->m_LogStream = &logStream;
  }

private:
  std::ostream & Logger() const
  {
    return *m_LogStream;
  }

  std::vector<unsigned int> m_NumberOfIterations;
  std::ostream *            m_LogStream;
  itk::TimeProbe            m_clock;

  itk::RealTimeClock::TimeStampType m_lastTotalTime;
};

/** \class antsRegistrationOptimizerCommandIterationUpdate
 *  \brief observe the optimizer for traditional registration methods
 */
template <class TOptimizer>
class antsRegistrationOptimizerCommandIterationUpdate : public itk::Command
{
public:
  typedef antsRegistrationOptimizerCommandIterationUpdate Self;
  typedef itk::Command                                    Superclass;
  typedef itk::SmartPointer<Self>                         Pointer;
  itkNewMacro( Self );
protected:
  antsRegistrationOptimizerCommandIterationUpdate()
  {
    m_clock.Start();
    m_clock.Stop();
    const itk::RealTimeClock::TimeStampType now=m_clock.GetTotal ();
    this->m_lastTotalTime=now;
    m_clock.Start();
    this->m_LogStream = &::ants::antscout;
  }

public:

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *) caller, event);
  }

  void Execute(const itk::Object *, const itk::EventObject & event)
  {
    if( typeid( event ) == typeid( itk::IterationEvent ) )
      {
      const unsigned int lCurrentIteration=this->m_Optimizer->GetCurrentIteration() + 1;
      if ( lCurrentIteration  == 1 )
	{
	//Print header line one time
	this->Logger() << "DIAGNOSTIC,Iteration,metricValue,convergenceValue,ITERATION_TIME_INDEX,SINCE_LAST" << std::endl;

	}
      m_clock.Stop();
      const itk::RealTimeClock::TimeStampType now=m_clock.GetTotal ();
      this->Logger() << "DIAGNOSTIC, "
                     << lCurrentIteration << ", "
                     << this->m_Optimizer->GetValue() << ", "
                     << this->m_Optimizer->GetConvergenceValue() << ", "
                     << now << ", "
		     << (now-this->m_lastTotalTime)  << ", "
                     << std::endl;
      this->m_lastTotalTime=now;
      m_clock.Start();
      }
  }

  void SetLogStream(std::ostream & logStream)
  {
    this->m_LogStream = &logStream;
  }

  /**
   * Type defining the optimizer
   */
  typedef    TOptimizer OptimizerType;

  /**
   * Set Optimizer
   */
  void SetOptimizer( OptimizerType * optimizer )
  {
    this->m_Optimizer = optimizer;
    this->m_Optimizer->AddObserver( itk::IterationEvent(), this );
  }

private:
  /**
   *  WeakPointer to the Optimizer
   */
  itk::WeakPointer<OptimizerType> m_Optimizer;

  std::ostream & Logger() const
  {
    return *m_LogStream;
  }

  std::ostream *m_LogStream;
  itk::TimeProbe m_clock;
  itk::RealTimeClock::TimeStampType m_lastTotalTime;
};

/**
 * Transform traits to generalize the rigid transform
 */
template <unsigned int ImageDimension>
class RigidTransformTraits
{

// Don't worry about the fact that the default option is the
// affine Transform, that one will not actually be instantiated.
public:
  typedef itk::AffineTransform<double, ImageDimension> TransformType;
};

template <>
class RigidTransformTraits<2>
{
public:
  typedef itk::Euler2DTransform<double> TransformType;
};

template <>
class RigidTransformTraits<3>
{
public:
  // typedef itk::VersorRigid3DTransform<double>    TransformType;
  // typedef itk::QuaternionRigidTransform<double>  TransformType;
  typedef itk::Euler3DTransform<double> TransformType;
};

template <unsigned int ImageDimension>
class SimilarityTransformTraits
{
// Don't worry about the fact that the default option is the
// affine Transform, that one will not actually be instantiated.
public:
  typedef itk::AffineTransform<double, ImageDimension> TransformType;
};

template <>
class SimilarityTransformTraits<2>
{
public:
  typedef itk::Similarity2DTransform<double> TransformType;
};

template <>
class SimilarityTransformTraits<3>
{
public:
  typedef itk::Similarity3DTransform<double> TransformType;
};

template <unsigned int ImageDimension>
class CompositeAffineTransformTraits
{
// Don't worry about the fact that the default option is the
// affine Transform, that one will not actually be instantiated.
public:
  typedef itk::AffineTransform<double, ImageDimension> TransformType;
};
template <>
class CompositeAffineTransformTraits<2>
{
public:
  typedef itk::ANTSCenteredAffine2DTransform<double> TransformType;
};
template <>
class CompositeAffineTransformTraits<3>
{
public:
  typedef itk::ANTSAffine3DTransform<double> TransformType;
};

template <unsigned VImageDimension>
RegistrationHelper<VImageDimension>
::RegistrationHelper() :
  m_CompositeTransform( NULL ),
  m_FixedInitialTransform( NULL ),
  m_NumberOfStages( 0 ),
  m_Metrics(),
  m_TransformMethods(),
  m_Iterations(),
  m_SmoothingSigmas(),
  m_ShrinkFactors(),
  m_UseHistogramMatching( true ),
  m_WinsorizeImageIntensities( false ),
  m_DoEstimateLearningRateAtEachIteration( true ),
  m_LowerQuantile( 0.0 ),
  m_UpperQuantile( 1.0 ),
  m_LogStream( &::ants::antscout ),
  m_ApplyLinearTransformsToFixedImageHeader( true ),
  m_AllPreviousTransformsAreLinear( true ),
  m_CompositeLinearTransformForFixedImageHeader( NULL )
{
  typedef itk::LinearInterpolateImageFunction<ImageType, RealType> LinearInterpolatorType;
  typename LinearInterpolatorType::Pointer linearInterpolator = LinearInterpolatorType::New();
  this->m_Interpolator = linearInterpolator;
}

template <unsigned VImageDimension>
RegistrationHelper<VImageDimension>
::~RegistrationHelper()
{
}

template <class ImageType>
typename ImageType::Pointer PreprocessImage( typename ImageType::ConstPointer  inputImage,
                                             typename ImageType::PixelType lowerScaleValue,
                                             typename ImageType::PixelType upperScaleValue,
                                             float winsorizeLowerQuantile, float winsorizeUpperQuantile,
                                             typename ImageType::ConstPointer histogramMatchSourceImage = NULL )
{
  typedef itk::Statistics::ImageToHistogramFilter<ImageType>   HistogramFilterType;
  typedef typename HistogramFilterType::InputBooleanObjectType InputBooleanObjectType;
  typedef typename HistogramFilterType::HistogramSizeType      HistogramSizeType;
  typedef typename HistogramFilterType::HistogramType          HistogramType;

  HistogramSizeType histogramSize( 1 );
  histogramSize[0] = 256;

  typename InputBooleanObjectType::Pointer autoMinMaxInputObject = InputBooleanObjectType::New();
  autoMinMaxInputObject->Set( true );

  typename HistogramFilterType::Pointer histogramFilter = HistogramFilterType::New();
  histogramFilter->SetInput( inputImage );
  histogramFilter->SetAutoMinimumMaximumInput( autoMinMaxInputObject );
  histogramFilter->SetHistogramSize( histogramSize );
  histogramFilter->SetMarginalScale( 10.0 );
  histogramFilter->Update();

  float lowerValue = histogramFilter->GetOutput()->Quantile( 0, winsorizeLowerQuantile );
  float upperValue = histogramFilter->GetOutput()->Quantile( 0, winsorizeUpperQuantile );

  typedef itk::IntensityWindowingImageFilter<ImageType, ImageType> IntensityWindowingImageFilterType;

  typename IntensityWindowingImageFilterType::Pointer windowingFilter = IntensityWindowingImageFilterType::New();
  windowingFilter->SetInput( inputImage );
  windowingFilter->SetWindowMinimum( lowerValue );
  windowingFilter->SetWindowMaximum( upperValue );
  windowingFilter->SetOutputMinimum( lowerScaleValue );
  windowingFilter->SetOutputMaximum( upperScaleValue );
  windowingFilter->Update();

  typename ImageType::Pointer outputImage = NULL;
  if( histogramMatchSourceImage )
    {
    typedef itk::HistogramMatchingImageFilter<ImageType, ImageType> HistogramMatchingFilterType;
    typename HistogramMatchingFilterType::Pointer matchingFilter = HistogramMatchingFilterType::New();
    matchingFilter->SetSourceImage( windowingFilter->GetOutput() );
    matchingFilter->SetReferenceImage( histogramMatchSourceImage );
    matchingFilter->SetNumberOfHistogramLevels( 256 );
    matchingFilter->SetNumberOfMatchPoints( 12 );
    matchingFilter->ThresholdAtMeanIntensityOn();
    matchingFilter->Update();

    outputImage = matchingFilter->GetOutput();
    outputImage->Update();
    outputImage->DisconnectPipeline();
    }
  else
    {
    outputImage = windowingFilter->GetOutput();
    outputImage->Update();
    outputImage->DisconnectPipeline();
    }
  return outputImage;
}

template <unsigned VImageDimension>
typename RegistrationHelper<VImageDimension>::MetricEnumeration
RegistrationHelper<VImageDimension>
::StringToMetricType(const std::string & str) const
{
  if( str == "cc" )
    {
    return CC;
    }
  else if( str == "mi2" )
    {
    return MI;
    }
  else if( str == "mattes" || str == "mi" )
    {
    return Mattes;
    }
  else if( str == "meansquares" || str == "msq" || str == "ssd" )
    {
    return MeanSquares;
    }
  else if( str == "demons" )
    {
    return Demons;
    }
  else if( str == "gc" )
    {
    return GC;
    }
  return IllegalMetric;
}

template <unsigned VImageDimension>
typename RegistrationHelper<VImageDimension>::XfrmMethod
RegistrationHelper<VImageDimension>
::StringToXfrmMethod(const std::string & str) const
{
  if( str == "rigid" )
    {
    return Rigid;
    }
  else if( str == "affine" )
    {
    return Affine;
    }
  if( str == "compositeaffine" || str == "compaff" )
    {
    return CompositeAffine;
    }
  if( str == "similarity" )
    {
    return Similarity;
    }
  if( str == "translation" )
    {
    return Translation;
    }
  if( str == "bspline" ||
      str == "ffd" )
    {
    return BSpline;
    }
  if( str == "gaussiandisplacementfield" ||
      str == "gdf" )
    {
    return GaussianDisplacementField;
    }
  if( str == "bsplinedisplacementfield" ||
      str == "dmffd" )
    {
    return BSplineDisplacementField;
    }
  if( str == "timevaryingvelocityfield" ||
      str == "tvf" )
    {
    return TimeVaryingVelocityField;
    }
  if( str == "timevaryingbsplinevelocityfield" ||
      str == "tvdmffd" )
    {
    return TimeVaryingBSplineVelocityField;
    }
  if( str == "syn" ||
      str == "symmetricnormalization" )
    {
    return SyN;
    }
  if( str == "bsplinesyn" )
    {
    return BSplineSyN;
    }
  if( str == "exp" ||
      str == "exponential" )
    {
    return Exponential;
    }
  if( str == "bsplineexponential" )
    {
    return BSplineExponential;
    }
  return UnknownXfrm;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddMetric(MetricEnumeration metricType,
            typename ImageType::Pointer & fixedImage,
            typename ImageType::Pointer & movingImage,
            double weighting,
            SamplingStrategy samplingStrategy,
            int numberOfBins,
            unsigned int  radius,
            double samplingPercentage)
{
  Metric init(metricType, fixedImage, movingImage,
              weighting, samplingStrategy, numberOfBins,
              radius,
              samplingPercentage);

  this->m_Metrics.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddRigidTransform(double GradientStep)
{
  TransformMethod init;

  init.m_XfrmMethod = Rigid;
  init.m_GradientStep = GradientStep;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddAffineTransform(double GradientStep)
{
  TransformMethod init;

  init.m_XfrmMethod = Affine;
  init.m_GradientStep = GradientStep;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddCompositeAffineTransform(double GradientStep)
{
  TransformMethod init;

  init.m_XfrmMethod = CompositeAffine;
  init.m_GradientStep = GradientStep;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddSimilarityTransform(double GradientStep)
{
  TransformMethod init;

  init.m_XfrmMethod = Similarity;
  init.m_GradientStep = GradientStep;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddTranslationTransform(double GradientStep)
{
  TransformMethod init;

  init.m_XfrmMethod = Translation;
  init.m_GradientStep = GradientStep;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddBSplineTransform(double GradientStep, std::vector<unsigned int> & MeshSizeAtBaseLevel)
{
  TransformMethod init;

  init.m_XfrmMethod = BSpline;
  init.m_GradientStep = GradientStep;
  init.m_MeshSizeAtBaseLevel = MeshSizeAtBaseLevel;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddGaussianDisplacementFieldTransform(double GradientStep, double UpdateFieldVarianceInVarianceSpace,
                                        double TotalFieldVarianceInVarianceSpace)
{
  TransformMethod init;

  init.m_XfrmMethod = GaussianDisplacementField;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldVarianceInVarianceSpace = UpdateFieldVarianceInVarianceSpace;
  init.m_TotalFieldVarianceInVarianceSpace = TotalFieldVarianceInVarianceSpace;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddBSplineDisplacementFieldTransform(double GradientStep,
                                       std::vector<unsigned int> & UpdateFieldMeshSizeAtBaseLevel,
                                       std::vector<unsigned int> & TotalFieldMeshSizeAtBaseLevel,
                                       unsigned int SplineOrder)
{
  TransformMethod init;

  init.m_XfrmMethod = BSplineDisplacementField;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldMeshSizeAtBaseLevel = UpdateFieldMeshSizeAtBaseLevel;
  init.m_TotalFieldMeshSizeAtBaseLevel = TotalFieldMeshSizeAtBaseLevel;
  init.m_SplineOrder = SplineOrder;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddTimeVaryingVelocityFieldTransform(double GradientStep,
                                       unsigned int NumberOfTimeIndices,
                                       double UpdateFieldVarianceInVarianceSpace,
                                       double UpdateFieldTimeSigma,
                                       double TotalFieldVarianceInVarianceSpace,
                                       double TotalFieldTimeSigma)
{
  TransformMethod init;

  init.m_XfrmMethod = TimeVaryingVelocityField;
  init.m_GradientStep = GradientStep;
  init.m_NumberOfTimeIndices = NumberOfTimeIndices;
  init.m_UpdateFieldVarianceInVarianceSpace = UpdateFieldVarianceInVarianceSpace;
  init.m_UpdateFieldTimeSigma = UpdateFieldTimeSigma;
  init.m_TotalFieldVarianceInVarianceSpace = TotalFieldVarianceInVarianceSpace;
  init.m_TotalFieldTimeSigma = TotalFieldTimeSigma;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddTimeVaryingBSplineVelocityFieldTransform(double GradientStep, std::vector<unsigned int> VelocityFieldMeshSize,
                                              unsigned int NumberOfTimePointSamples, unsigned int SplineOrder)
{
  TransformMethod init;

  init.m_XfrmMethod = TimeVaryingBSplineVelocityField;;
  init.m_GradientStep = GradientStep;
  init.m_VelocityFieldMeshSize = VelocityFieldMeshSize;
  init.m_NumberOfTimePointSamples = NumberOfTimePointSamples;
  init.m_SplineOrder = SplineOrder;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddSyNTransform(double GradientStep, double UpdateFieldVarianceInVarianceSpace,
                  double TotalFieldVarianceInVarianceSpace)
{
  TransformMethod init;

  init.m_XfrmMethod = SyN;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldVarianceInVarianceSpace = UpdateFieldVarianceInVarianceSpace;
  init.m_TotalFieldVarianceInVarianceSpace = TotalFieldVarianceInVarianceSpace;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddBSplineSyNTransform(double GradientStep, std::vector<unsigned int> &  UpdateFieldMeshSizeAtBaseLevel,
                         std::vector<unsigned int> &  TotalFieldMeshSizeAtBaseLevel,
                         unsigned int SplineOrder)
{
  TransformMethod init;

  init.m_XfrmMethod = BSplineSyN;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldMeshSizeAtBaseLevel = UpdateFieldMeshSizeAtBaseLevel;
  init.m_TotalFieldMeshSizeAtBaseLevel = TotalFieldMeshSizeAtBaseLevel;
  init.m_SplineOrder = SplineOrder;
  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddExponentialTransform(double GradientStep, double UpdateFieldVarianceInVarianceSpace,
                          double VelocityFieldVarianceInVarianceSpace, unsigned int NumberOfIntegrationSteps)
{
  TransformMethod init;

  init.m_XfrmMethod = Exponential;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldVarianceInVarianceSpace = UpdateFieldVarianceInVarianceSpace;
  init.m_VelocityFieldVarianceInVarianceSpace = VelocityFieldVarianceInVarianceSpace;
  init.m_NumberOfTimeIndices = NumberOfIntegrationSteps;

  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::AddBSplineExponentialTransform(double GradientStep, std::vector<unsigned int> &  UpdateFieldMeshSizeAtBaseLevel,
                                 std::vector<unsigned int> & VelocityFieldMeshSizeAtBaseLevel, unsigned int NumberOfIntegrationSteps,
                                 unsigned int SplineOrder)
{
  TransformMethod init;

  init.m_XfrmMethod = BSplineExponential;
  init.m_GradientStep = GradientStep;
  init.m_UpdateFieldMeshSizeAtBaseLevel = UpdateFieldMeshSizeAtBaseLevel;
  init.m_VelocityFieldMeshSizeAtBaseLevel = VelocityFieldMeshSizeAtBaseLevel;
  init.m_SplineOrder = SplineOrder;
  init.m_NumberOfTimeIndices = NumberOfIntegrationSteps;

  this->m_TransformMethods.push_back(init);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetIterations(const std::vector<std::vector<unsigned int> > & Iterations)
{
  this->m_Iterations = Iterations;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetConvergenceThresholds(const std::vector<double> & thresholds)
{
  this->m_ConvergenceThresholds = thresholds;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetConvergenceWindowSizes(const std::vector<unsigned int> & windowSizes)
{
  this->m_ConvergenceWindowSizes = windowSizes;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetSmoothingSigmas(const std::vector<std::vector<float> > & SmoothingSigmas)
{
  this->m_SmoothingSigmas = SmoothingSigmas;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetShrinkFactors(const std::vector<std::vector<unsigned int> > & ShrinkFactors)
{
  this->m_ShrinkFactors = ShrinkFactors;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetWinsorizeImageIntensities(bool Winsorize, float LowerQuantile, float UpperQuantile)
{
  this->m_WinsorizeImageIntensities = Winsorize;
  this->m_LowerQuantile = LowerQuantile;
  this->m_UpperQuantile = UpperQuantile;
}

template <unsigned VImageDimension>
int
RegistrationHelper<VImageDimension>
::ValidateParameters()
{
  if( this->m_NumberOfStages == 0 )
    {
    ::ants::antscout << "No transformations are specified." << std::endl;
    return EXIT_FAILURE;
    }
  if( this->m_Metrics.size() != this->m_NumberOfStages )
    {
    ::ants::antscout << "The number of metrics specified does not match the number of stages. ["
                     << this->m_Metrics.size()  << " != " << this->m_NumberOfStages << "]" << std::endl;
    return EXIT_FAILURE;
    }
  if( this->m_Iterations.size() != this->m_NumberOfStages )
    {
    ::ants::antscout << "The number of iteration sets specified does not match the number of stages." << std::endl;
    return EXIT_FAILURE;
    }
  if( this->m_ShrinkFactors.size() != this->m_NumberOfStages )
    {
    ::ants::antscout << "The number of shrinkFactors specified does not match the number of stages." << std::endl;
    return EXIT_FAILURE;
    }
  if( this->m_SmoothingSigmas.size() != this->m_NumberOfStages )
    {
    ::ants::antscout << "The number of smoothing sigma sets specified does not match the number of stages."
                     << std::endl;
    return EXIT_FAILURE;
    }
  for( unsigned int i = 0; i < this->m_NumberOfStages; i++ )
    {
    if( this->m_Metrics[i].m_FixedImage.IsNull() ||
        this->m_Metrics[i].m_MovingImage.IsNull() )
      {
      ::ants::antscout << "Must either add Metrics with filenames, or pointers to images" << std::endl;
      return EXIT_FAILURE;
      }
    }
  return EXIT_SUCCESS;
}

template <unsigned VImageDimension>
typename RegistrationHelper<VImageDimension>::ImageType::Pointer
RegistrationHelper<VImageDimension>
::GetWarpedImage() const
{
  typename ImageType::Pointer fixedImage = this->m_Metrics[0].m_FixedImage;
  typename ImageType::Pointer movingImage = this->m_Metrics[0].m_MovingImage;

  typedef itk::ResampleImageFilter<ImageType, ImageType> ResampleFilterType;
  typename ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetTransform( this->m_CompositeTransform );
  resampler->SetInput( movingImage );
  resampler->SetOutputParametersFromImage( fixedImage );
  resampler->SetInterpolator( this->m_Interpolator );
  resampler->SetDefaultPixelValue( 0 );
  resampler->Update();

  typename ImageType::Pointer  WarpedImage;
  WarpedImage = resampler->GetOutput();
  return WarpedImage.GetPointer();
}

template <unsigned VImageDimension>
typename RegistrationHelper<VImageDimension>::ImageType::Pointer
RegistrationHelper<VImageDimension>
::GetInverseWarpedImage() const
{
  typename ImageType::Pointer fixedImage = this->m_Metrics[0].m_FixedImage;
  typename ImageType::Pointer movingImage = this->m_Metrics[0].m_MovingImage;

  if( this->m_CompositeTransform->GetInverseTransform().IsNull() )
    {
    return 0;
    }
  typedef itk::ResampleImageFilter<ImageType, ImageType> ResampleFilterType;
  typename ResampleFilterType::Pointer inverseResampler = ResampleFilterType::New();
  inverseResampler->SetTransform( this->m_CompositeTransform->GetInverseTransform() );
  inverseResampler->SetInput( fixedImage );
  inverseResampler->SetOutputParametersFromImage( movingImage );
  inverseResampler->SetInterpolator( this->m_Interpolator );
  inverseResampler->SetDefaultPixelValue( 0 );
  inverseResampler->Update();

  typename ImageType::Pointer InverseWarpedImage;
  InverseWarpedImage = inverseResampler->GetOutput();
  return InverseWarpedImage.GetPointer();
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetFixedImageMask(typename MaskImageType::Pointer & fixedImageMask)
{
  typename ImageMaskSpatialObjectType::Pointer so =
    ImageMaskSpatialObjectType::New();
  so->SetImage(fixedImageMask.GetPointer() );
  this->SetFixedImageMask(so);
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetMovingImageMask(typename MaskImageType::Pointer & movingImageMask)
{
  typename ImageMaskSpatialObjectType::Pointer so =
    ImageMaskSpatialObjectType::New();
  so->SetImage(movingImageMask.GetPointer() );
  this->SetMovingImageMask(so);
}

template <unsigned VImageDimension>
int
RegistrationHelper<VImageDimension>
::DoRegistration()
{
  itk::TimeProbe totalTimer;

  totalTimer.Start();

  this->m_NumberOfStages = this->m_TransformMethods.size();

  if( this->ValidateParameters() != EXIT_SUCCESS )
    {
    return EXIT_FAILURE;
    }
  this->PrintState();

  this->Logger() << "Registration using " << this->m_NumberOfStages << " total stages." << std::endl;

  // NOTE:  the -1 is to ignore the initial identity identity transform
  if( this->m_CompositeTransform.IsNull() )
    {
    this->m_CompositeTransform = CompositeTransformType::New();
    }
  if( this->m_CompositeLinearTransformForFixedImageHeader.IsNull() )
    {
    this->m_CompositeLinearTransformForFixedImageHeader = CompositeTransformType::New();
    }
  if( this->m_FixedInitialTransform.IsNull() )
    {
    this->m_FixedInitialTransform = CompositeTransformType::New();
    }

  // ########################################################################################
  // ########################################################################################
  // ## The main loop for exstimating the total composite tranform
  // ########################################################################################
  // ########################################################################################
  for( unsigned int currentStageNumber = 0; currentStageNumber < this->m_NumberOfStages; currentStageNumber++ )
    {
    itk::TimeProbe timer;
    timer.Start();

    typedef itk::ImageRegistrationMethodv4<ImageType, ImageType> AffineRegistrationType;

    this->Logger() << std::endl << "Stage " << currentStageNumber << std::endl;
    std::stringstream currentStageString;
    currentStageString << currentStageNumber;

    // Get the fixed and moving images
    const typename ImageType::ConstPointer fixedImage = this->m_Metrics[currentStageNumber].m_FixedImage.GetPointer();
    const typename ImageType::ConstPointer movingImage = this->m_Metrics[currentStageNumber].m_MovingImage.GetPointer();
    // Preprocess images

    std::string outputPreprocessingString = "";

    PixelType lowerScaleValue = 0.0;
    PixelType upperScaleValue = 1.0;
    if( this->m_WinsorizeImageIntensities )
      {
      outputPreprocessingString += "  preprocessing:  winsorizing the image intensities\n";
      }

    const typename ImageType::Pointer preprocessFixedImage =
      PreprocessImage<ImageType>( fixedImage.GetPointer(), lowerScaleValue,
                                  upperScaleValue, this->m_LowerQuantile, this->m_UpperQuantile,
                                  NULL );

    typename ImageType::ConstPointer preprocessMovingImage;

    if( this->m_UseHistogramMatching )
      {
      outputPreprocessingString += "  preprocessing:  histogram matching the images\n";
      preprocessMovingImage =
        PreprocessImage<ImageType>( movingImage.GetPointer(),
                                    lowerScaleValue, upperScaleValue,
                                    this->m_LowerQuantile, this->m_UpperQuantile,
                                    preprocessFixedImage.GetPointer() ).GetPointer();
      }
    else
      {
      preprocessMovingImage =
        PreprocessImage<ImageType>( movingImage.GetPointer(),
                                    lowerScaleValue, upperScaleValue,
                                    this->m_LowerQuantile, this->m_UpperQuantile,
                                    NULL ).GetPointer();
      }

    if( this->m_ApplyLinearTransformsToFixedImageHeader )
      {
      this->ApplyCompositeLinearTransformToImageHeader( this->m_CompositeLinearTransformForFixedImageHeader,
                                                        dynamic_cast<ImageBaseType *>( preprocessFixedImage.GetPointer() ), false );

      if( this->m_FixedImageMask.IsNotNull() )
        {
        this->ApplyCompositeLinearTransformToImageHeader( this->m_CompositeLinearTransformForFixedImageHeader,
                                                          dynamic_cast<ImageBaseType *>( const_cast<MaskImageType *>( this->m_FixedImageMask->GetImage() ) ), false );
        }
      }

    this->Logger() << outputPreprocessingString << std::flush;

    // Get the number of iterations and use that information to specify the number of levels

    const std::vector<unsigned int> & currentStageIterations = this->m_Iterations[currentStageNumber];
    this->Logger() << "  iterations = ";
    for( unsigned int m = 0; m < currentStageIterations.size(); m++ )
      {
      this->Logger() << currentStageIterations[m];
      if( m < currentStageIterations.size() - 1 )
        {
        this->Logger() << 'x';
        }
      }
    this->Logger() << std::endl;

    const double convergenceThreshold = this->m_ConvergenceThresholds[currentStageNumber];
    this->Logger() << "  convergence threshold = " << convergenceThreshold << std::endl;
    const unsigned int convergenceWindowSize = this->m_ConvergenceWindowSizes[currentStageNumber];
    this->Logger() << "  convergence window size = " << convergenceWindowSize << std::endl;

    const unsigned int numberOfLevels = currentStageIterations.size();
    this->Logger() << "  number of levels = " << numberOfLevels << std::endl;

    // Get shrink factors
    const std::vector<unsigned int> factors(this->m_ShrinkFactors[currentStageNumber]);
    typename AffineRegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
    shrinkFactorsPerLevel.SetSize( factors.size() );

    if( factors.size() != numberOfLevels )
      {
      ::ants::antscout << "\n\n\n"
                       << "ERROR:  The number of shrink factors does not match the number of levels."
                       << "\nShrink Factors: " << factors.size()
                       << "\nNumber Of Levels: " << numberOfLevels
                       << "\n\n\n"
                       << std::endl;
      return EXIT_FAILURE;
      }
    for( unsigned int n = 0; n < shrinkFactorsPerLevel.Size(); n++ )
      {
      shrinkFactorsPerLevel[n] = factors[n];
      }
    this->Logger() << "  shrink factors per level: " << shrinkFactorsPerLevel << std::endl;

    // Get smoothing sigmas
    const std::vector<float> sigmas( this->m_SmoothingSigmas[currentStageNumber] );
    typename AffineRegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
    smoothingSigmasPerLevel.SetSize( sigmas.size() );

    if( sigmas.size() != numberOfLevels )
      {
      ::ants::antscout << "ERROR:  The number of smoothing sigmas "
                       << "does not match the number of levels." << std::endl;
      return EXIT_FAILURE;
      }
    for( unsigned int n = 0; n < smoothingSigmasPerLevel.Size(); n++ )
      {
      smoothingSigmasPerLevel[n] = sigmas[n];
      }
    this->Logger() << "  smoothing sigmas per level: " << smoothingSigmasPerLevel << std::endl;

    // Set up the image metric and scales estimator

    typename MetricType::Pointer metric;

    const float            samplingPercentage = this->m_Metrics[currentStageNumber].m_SamplingPercentage;
    const SamplingStrategy samplingStrategy = this->m_Metrics[currentStageNumber].m_SamplingStrategy;
    typename AffineRegistrationType::MetricSamplingStrategyType metricSamplingStrategy = AffineRegistrationType::NONE;
    if( samplingStrategy == random )
      {
      this->Logger() << "  random sampling (percentage = " << samplingPercentage << ")" << std::endl;
      metricSamplingStrategy = AffineRegistrationType::RANDOM;
      }
    else if( samplingStrategy == regular )
      {
      this->Logger() << "  regular sampling (percentage = " << samplingPercentage << ")" << std::endl;
      metricSamplingStrategy = AffineRegistrationType::REGULAR;
      }
    else if( samplingStrategy == none )
      {
      ::ants::antscout << "  Using default NONE metricSamplingStrategy " << std::endl;
      }
    else
      {
      std::cout << "ERROR: samplingStrategy is incorrectly specified" << std::endl;
      exit(-1);
      }

    switch( this->m_Metrics[currentStageNumber].m_MetricType )
      {
      case CC:
        {
        const unsigned int radiusOption = this->m_Metrics[currentStageNumber].m_Radius;
        this->Logger() << "  using the CC metric (radius = "
                       << radiusOption << ")" << std::endl;
        typedef itk::ANTSNeighborhoodCorrelationImageToImageMetricv4<ImageType, ImageType> CorrelationMetricType;
        typename CorrelationMetricType::Pointer correlationMetric = CorrelationMetricType::New();
          {
          typename CorrelationMetricType::RadiusType radius;
          radius.Fill( radiusOption );
          correlationMetric->SetRadius( radius );
          }
        correlationMetric->SetUseMovingImageGradientFilter( false );
        correlationMetric->SetUseFixedImageGradientFilter( false );

        metric = correlationMetric;
        }
        break;
      case Mattes:
        {
        const unsigned int binOption = this->m_Metrics[currentStageNumber].m_NumberOfBins;
        this->Logger() << "  using the Mattes MI metric (number of bins = "
                       << binOption << ")" << std::endl;
        typedef itk::MattesMutualInformationImageToImageMetricv4<ImageType, ImageType>
        MutualInformationMetricType;
        typename MutualInformationMetricType::Pointer mutualInformationMetric =
          MutualInformationMetricType::New();
        mutualInformationMetric = mutualInformationMetric;
        mutualInformationMetric->SetNumberOfHistogramBins( binOption );
        mutualInformationMetric->SetUseMovingImageGradientFilter( false );
        mutualInformationMetric->SetUseFixedImageGradientFilter( false );
        mutualInformationMetric->SetUseFixedSampledPointSet( false );
        metric = mutualInformationMetric;
        }
        break;
      case MI:
        {
        const unsigned int binOption = this->m_Metrics[currentStageNumber].m_NumberOfBins;

        this->Logger() << "  using the MI metric (number of bins = " << binOption << ")" << std::endl;
        typedef itk::JointHistogramMutualInformationImageToImageMetricv4<ImageType,
                                                                         ImageType> MutualInformationMetricType;
        typename MutualInformationMetricType::Pointer mutualInformationMetric =
          MutualInformationMetricType::New();
        mutualInformationMetric = mutualInformationMetric;
        mutualInformationMetric->SetNumberOfHistogramBins( binOption );
        mutualInformationMetric->SetUseMovingImageGradientFilter( false );
        mutualInformationMetric->SetUseFixedImageGradientFilter( false );
        mutualInformationMetric->SetUseFixedSampledPointSet( false );
        mutualInformationMetric->SetVarianceForJointPDFSmoothing( 1.0 );
        metric = mutualInformationMetric;
        }
        break;
      case MeanSquares:
        {
        this->Logger() << "  using the MeanSquares metric." << std::endl;

        typedef itk::MeanSquaresImageToImageMetricv4<ImageType, ImageType> MeanSquaresMetricType;
        typename MeanSquaresMetricType::Pointer meanSquaresMetric = MeanSquaresMetricType::New();
        meanSquaresMetric = meanSquaresMetric;
        metric = meanSquaresMetric;
        }
        break;
      case Demons:
        {
        this->Logger() << "  using the Demons metric." << std::endl;

        typedef itk::DemonsImageToImageMetricv4<ImageType, ImageType> DemonsMetricType;
        typename DemonsMetricType::Pointer demonsMetric = DemonsMetricType::New();
        demonsMetric = demonsMetric;
        metric = demonsMetric;
        }
        break;
      case GC:
        {
        this->Logger() << "  using the global correlation metric." << std::endl;
        typedef itk::CorrelationImageToImageMetricv4<ImageType, ImageType> corrMetricType;
        typename corrMetricType::Pointer corrMetric = corrMetricType::New();
        metric = corrMetric;
        }
        break;
      default:
        ::ants::antscout << "ERROR: Unrecognized image metric: " << std::endl;
      }
    /** Can really impact performance */
    const bool gaussian = false;
    metric->SetUseMovingImageGradientFilter( gaussian );
    metric->SetUseFixedImageGradientFilter( gaussian );
    if( this->m_FixedImageMask.IsNotNull() )
      {
      metric->SetFixedImageMask(this->m_FixedImageMask);
      }
    if( this->m_MovingImageMask.IsNotNull() )
      {
      metric->SetMovingImageMask(this->m_MovingImageMask);
      }
    // Set up the optimizer.  To change the iteration number for each level we rely
    // on the command observer.

    const double learningRate = this->m_TransformMethods[currentStageNumber].m_GradientStep;

    typedef itk::RegistrationParameterScalesFromPhysicalShift<MetricType> ScalesEstimatorType;
    typename ScalesEstimatorType::Pointer scalesEstimator = ScalesEstimatorType::New();
    scalesEstimator->SetMetric( metric );
    scalesEstimator->SetTransformForward( true );

    typedef itk::ConjugateGradientLineSearchOptimizerv4 ConjugateGradientDescentOptimizerType;
    typename ConjugateGradientDescentOptimizerType::Pointer optimizer = ConjugateGradientDescentOptimizerType::New();
    optimizer->SetLowerLimit( 0 );
    optimizer->SetUpperLimit( 2 );
    optimizer->SetEpsilon( 0.2 );
    //    optimizer->SetMaximumLineSearchIterations( 20 );
    optimizer->SetLearningRate( learningRate );
    optimizer->SetMaximumStepSizeInPhysicalUnits( learningRate );
    optimizer->SetNumberOfIterations( currentStageIterations[0] );
    optimizer->SetScalesEstimator( scalesEstimator );
    optimizer->SetMinimumConvergenceValue( convergenceThreshold );
    optimizer->SetConvergenceWindowSize( convergenceWindowSize );
    optimizer->SetDoEstimateLearningRateAtEachIteration( this->m_DoEstimateLearningRateAtEachIteration );
    optimizer->SetDoEstimateLearningRateOnce( !this->m_DoEstimateLearningRateAtEachIteration );
    typedef antsRegistrationOptimizerCommandIterationUpdate<ConjugateGradientDescentOptimizerType> OptimizerCommandType;
    typename OptimizerCommandType::Pointer optimizerObserver = OptimizerCommandType::New();
    optimizerObserver->SetLogStream( *this->m_LogStream );
    optimizerObserver->SetOptimizer( optimizer );

    typedef itk::GradientDescentLineSearchOptimizerv4 GradientDescentLSOptimizerType;
    typedef itk::GradientDescentOptimizerv4 GradientDescentOptimizerType;
    typename GradientDescentOptimizerType::Pointer optimizer2 = GradientDescentOptimizerType::New();
    //    optimizer2->SetLowerLimit( 0 );
    //    optimizer2->SetUpperLimit( 2 );
    //    optimizer2->SetEpsilon( 0.2 );
    //    optimizer->SetMaximumLineSearchIterations( 20 );
    optimizer2->SetLearningRate( learningRate );
    optimizer2->SetMaximumStepSizeInPhysicalUnits( learningRate );
    optimizer2->SetNumberOfIterations( currentStageIterations[0] );
    optimizer2->SetScalesEstimator( NULL );
    optimizer2->SetMinimumConvergenceValue( convergenceThreshold );
    optimizer2->SetConvergenceWindowSize( convergenceWindowSize );
    optimizer2->SetDoEstimateLearningRateAtEachIteration( this->m_DoEstimateLearningRateAtEachIteration );
    optimizer2->SetDoEstimateLearningRateOnce( !this->m_DoEstimateLearningRateAtEachIteration );
    typedef antsRegistrationOptimizerCommandIterationUpdate<GradientDescentOptimizerType> OptimizerCommandType2;
    typename OptimizerCommandType2::Pointer optimizerObserver2 = OptimizerCommandType2::New();
    optimizerObserver2->SetLogStream( *this->m_LogStream );
    optimizerObserver2->SetOptimizer( optimizer2 );

    // Set up the image registration methods along with the transforms
    const XfrmMethod whichTransform(this->m_TransformMethods[currentStageNumber].m_XfrmMethod);
    switch( whichTransform )
      {
      case Affine:
        {
        typedef itk::AffineTransform<RealType, VImageDimension> AffineTransformType;
        typename AffineRegistrationType::Pointer affineRegistration = AffineRegistrationType::New();
        affineRegistration->SetFixedImage( preprocessFixedImage );
        affineRegistration->SetMovingImage( preprocessMovingImage );
        affineRegistration->SetNumberOfLevels( numberOfLevels );
        affineRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        affineRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        affineRegistration->SetMetric( metric );
        affineRegistration->SetMetricSamplingStrategy( metricSamplingStrategy );
        affineRegistration->SetMetricSamplingPercentage( samplingPercentage );
        affineRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          affineRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          affineRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<AffineRegistrationType> AffineCommandType;
        typename AffineCommandType::Pointer affineObserver = AffineCommandType::New();
        affineObserver->SetLogStream( *this->m_LogStream );
        affineObserver->SetNumberOfIterations( currentStageIterations );
        affineRegistration->AddObserver( itk::IterationEvent(), affineObserver );
        affineRegistration->AddObserver( itk::InitializeEvent(), affineObserver );
        try
          {
          this->Logger() << std::endl << "*** Running affine registration ***" << std::endl << std::endl;
          affineObserver->Execute( affineRegistration, itk::StartEvent() );
          affineRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform or add it to the composite transform
        // which is incorporated into the fixed image header.
        if( this->m_ApplyLinearTransformsToFixedImageHeader && this->m_AllPreviousTransformsAreLinear )
          {
          this->m_CompositeLinearTransformForFixedImageHeader->AddTransform( const_cast<AffineTransformType *>(
                                                                             affineRegistration->GetOutput()->Get() ) );
          }
        else
          {
          this->m_CompositeTransform->AddTransform( const_cast<AffineTransformType *>(
                                                    affineRegistration->GetOutput()->Get() ) );
          }
        }
        break;
      case Rigid:
        {
        typedef typename RigidTransformTraits<VImageDimension>::TransformType RigidTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType, RigidTransformType> RigidRegistrationType;
        typename RigidRegistrationType::Pointer rigidRegistration = RigidRegistrationType::New();

        rigidRegistration->SetFixedImage( preprocessFixedImage );
        rigidRegistration->SetMovingImage( preprocessMovingImage );
        rigidRegistration->SetNumberOfLevels( numberOfLevels );
        rigidRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        rigidRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        rigidRegistration->SetMetric( metric );
        rigidRegistration->SetMetricSamplingStrategy(
          static_cast<typename RigidRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        rigidRegistration->SetMetricSamplingPercentage( samplingPercentage );
        rigidRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          rigidRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          rigidRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<RigidRegistrationType> RigidCommandType;
        typename RigidCommandType::Pointer rigidObserver = RigidCommandType::New();
        rigidObserver->SetLogStream(*this->m_LogStream);
        rigidObserver->SetNumberOfIterations( currentStageIterations );

        rigidRegistration->AddObserver( itk::IterationEvent(), rigidObserver );
        rigidRegistration->AddObserver( itk::InitializeEvent(), rigidObserver );

        try
          {
          this->Logger() << std::endl << "*** Running rigid registration ***" << std::endl << std::endl;
          rigidObserver->Execute( rigidRegistration, itk::StartEvent() );
          rigidRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform or add it to the composite transform
        // which is incorporated into the fixed image header.
        if( this->m_ApplyLinearTransformsToFixedImageHeader && this->m_AllPreviousTransformsAreLinear )
          {
          this->m_CompositeLinearTransformForFixedImageHeader->AddTransform( const_cast<RigidTransformType *>(
                                                                                rigidRegistration->GetOutput()->Get() ) );
          }
        else
          {
          this->m_CompositeTransform->AddTransform( const_cast<RigidTransformType *>( rigidRegistration->GetOutput()->
                                                                                      Get() ) );
          }
        }
        break;
      case CompositeAffine:
        {
        typedef typename CompositeAffineTransformTraits<VImageDimension>::TransformType CompositeAffineTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               CompositeAffineTransformType> AffineRegistrationType;
        typename AffineRegistrationType::Pointer affineRegistration = AffineRegistrationType::New();

        affineRegistration->SetFixedImage( preprocessFixedImage );
        affineRegistration->SetMovingImage( preprocessMovingImage );
        affineRegistration->SetNumberOfLevels( numberOfLevels );
        affineRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        affineRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        affineRegistration->SetMetric( metric );
        affineRegistration->SetMetricSamplingStrategy(
          static_cast<typename AffineRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        affineRegistration->SetMetricSamplingPercentage( samplingPercentage );
        affineRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          affineRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          affineRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<AffineRegistrationType> AffineCommandType;
        typename AffineCommandType::Pointer affineObserver = AffineCommandType::New();
        affineObserver->SetLogStream(*this->m_LogStream);
        affineObserver->SetNumberOfIterations( currentStageIterations );

        affineRegistration->AddObserver( itk::IterationEvent(), affineObserver );
        affineRegistration->AddObserver( itk::InitializeEvent(), affineObserver );

        try
          {
          this->Logger() << std::endl << "*** Running composite affine registration ***" << std::endl << std::endl;
          affineObserver->Execute( affineRegistration, itk::StartEvent() );
          affineRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform or add it to the composite transform
        // which is incorporated into the fixed image header.
        if( this->m_ApplyLinearTransformsToFixedImageHeader && this->m_AllPreviousTransformsAreLinear )
          {
          this->m_CompositeLinearTransformForFixedImageHeader->AddTransform( const_cast<CompositeAffineTransformType *>(
                                                                                affineRegistration->GetOutput()->Get() ) );
          }
        else
          {
          this->m_CompositeTransform->AddTransform( const_cast<CompositeAffineTransformType *>( affineRegistration->
                                                                                                GetOutput()->Get() ) );
          }
        }
        break;
      case Similarity:
        {
        typedef typename SimilarityTransformTraits<VImageDimension>::TransformType SimilarityTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               SimilarityTransformType> SimilarityRegistrationType;
        typename SimilarityRegistrationType::Pointer similarityRegistration = SimilarityRegistrationType::New();

        similarityRegistration->SetFixedImage( preprocessFixedImage );
        similarityRegistration->SetMovingImage( preprocessMovingImage );
        similarityRegistration->SetNumberOfLevels( numberOfLevels );
        similarityRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        similarityRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        similarityRegistration->SetMetric( metric );
        similarityRegistration->SetMetricSamplingStrategy(
          static_cast<typename SimilarityRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        similarityRegistration->SetMetricSamplingPercentage( samplingPercentage );
        similarityRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          similarityRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          similarityRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<SimilarityRegistrationType> SimilarityCommandType;
        typename SimilarityCommandType::Pointer similarityObserver = SimilarityCommandType::New();
        similarityObserver->SetLogStream(*this->m_LogStream);
        similarityObserver->SetNumberOfIterations( currentStageIterations );

        similarityRegistration->AddObserver( itk::IterationEvent(), similarityObserver );
        similarityRegistration->AddObserver( itk::InitializeEvent(), similarityObserver );

        try
          {
          this->Logger() << std::endl << "*** Running similarity registration ***" << std::endl << std::endl;
          similarityObserver->Execute( similarityRegistration, itk::StartEvent() );
          similarityRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform or add it to the composite transform
        // which is incorporated into the fixed image header.
        if( this->m_ApplyLinearTransformsToFixedImageHeader && this->m_AllPreviousTransformsAreLinear )
          {
          this->m_CompositeLinearTransformForFixedImageHeader->AddTransform( const_cast<SimilarityTransformType *>(
                                                                                similarityRegistration->GetOutput()->
                                                                                Get() ) );
          }
        else
          {
          this->m_CompositeTransform->AddTransform( const_cast<SimilarityTransformType *>( similarityRegistration->
                                                                                           GetOutput()->Get() ) );
          }
        }
        break;
      case Translation:
        {
        typedef itk::TranslationTransform<RealType, VImageDimension> TranslationTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               TranslationTransformType> TranslationRegistrationType;
        typename TranslationRegistrationType::Pointer translationRegistration = TranslationRegistrationType::New();

        translationRegistration->SetFixedImage( preprocessFixedImage );
        translationRegistration->SetMovingImage( preprocessMovingImage );
        translationRegistration->SetNumberOfLevels( numberOfLevels );
        translationRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        translationRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        translationRegistration->SetMetric( metric );
        translationRegistration->SetMetricSamplingStrategy(
          static_cast<typename TranslationRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        translationRegistration->SetMetricSamplingPercentage( samplingPercentage );
        translationRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          translationRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          translationRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<TranslationRegistrationType> TranslationCommandType;
        typename TranslationCommandType::Pointer translationObserver = TranslationCommandType::New();
        translationObserver->SetLogStream(*this->m_LogStream);
        translationObserver->SetNumberOfIterations( currentStageIterations );

        translationRegistration->AddObserver( itk::IterationEvent(), translationObserver );
        translationRegistration->AddObserver( itk::InitializeEvent(), translationObserver );

        try
          {
          this->Logger() << std::endl << "*** Running translation registration ***" << std::endl << std::endl;
          translationObserver->Execute( translationRegistration, itk::StartEvent() );
          translationRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform or add it to the composite transform
        // which is incorporated into the fixed image header.
        if( this->m_ApplyLinearTransformsToFixedImageHeader && this->m_AllPreviousTransformsAreLinear )
          {
          this->m_CompositeLinearTransformForFixedImageHeader->AddTransform( const_cast<TranslationTransformType *>(
                                                                                translationRegistration->GetOutput()->
                                                                                Get() ) );
          }
        else
          {
          this->m_CompositeTransform->AddTransform( const_cast<TranslationTransformType *>( translationRegistration->
                                                                                            GetOutput()->Get() ) );
          }
        }
        break;
      case GaussianDisplacementField:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );
        typedef itk::Image<VectorType, VImageDimension> DisplacementFieldType;
        // ORIENTATION ALERT: Original code set image size to
        // fixedImage buffered region, & if fixedImage BufferedRegion
        // != LargestPossibleRegion, this code would be wrong.
        typename DisplacementFieldType::Pointer displacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typedef itk::GaussianSmoothingOnUpdateDisplacementFieldTransform<RealType,
                                                                         VImageDimension>
        GaussianDisplacementFieldTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               GaussianDisplacementFieldTransformType>
        DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename GaussianDisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<GaussianDisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );

        // Create the transform adaptors

        typedef itk::GaussianSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
          GaussianDisplacementFieldTransformType>
        DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;

        // Extract parameters

        RealType varianceForUpdateField = this->m_TransformMethods[currentStageNumber].m_UpdateFieldVarianceInVarianceSpace;
        RealType varianceForTotalField  = this->m_TransformMethods[currentStageNumber].m_TotalFieldVarianceInVarianceSpace;

        outputDisplacementFieldTransform->SetGaussianSmoothingVarianceForTheUpdateField( varianceForUpdateField );
        outputDisplacementFieldTransform->SetGaussianSmoothingVarianceForTheTotalField( varianceForTotalField );
        outputDisplacementFieldTransform->SetDisplacementField( displacementField );
        // Create the transform adaptors
        // For the gaussian displacement field, the specified variances are in image spacing terms
        // and, in normal practice, we typically don't change these values at each level.  However,
        // if the user wishes to add that option, they can use the class
        // GaussianSmoothingOnUpdateDisplacementFieldTransformAdaptor
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.

          typedef itk::ShrinkImageFilter<DisplacementFieldType, DisplacementFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( displacementField );
          shrinkFilter->Update();

          typename DisplacementFieldTransformAdaptorType::Pointer fieldTransformAdaptor =
            DisplacementFieldTransformAdaptorType::New();
          fieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          fieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          fieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          fieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          fieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          adaptors.push_back( fieldTransformAdaptor.GetPointer() );
          }

        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename DisplacementFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        displacementFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        displacementFieldRegistration->SetOptimizer( optimizer );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running gaussian displacement field registration (varianceForUpdateField = "
                         << varianceForUpdateField << ", varianceForTotalField = " << varianceForTotalField << ") ***"
                         << std::endl << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case BSplineDisplacementField:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );
        typedef itk::Image<VectorType, VImageDimension> DisplacementFieldType;
        // ORIENTATION ALERT -- see comment above.
        typename DisplacementFieldType::Pointer displacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransform<RealType,
                                                                        VImageDimension>
        BSplineDisplacementFieldTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               BSplineDisplacementFieldTransformType> DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename BSplineDisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<BSplineDisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );
        outputDisplacementFieldTransform->SetDisplacementField( displacementField );

        // Create the transform adaptors

        typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
          BSplineDisplacementFieldTransformType>
        DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;

        // Extract parameters

        const std::vector<unsigned int> & meshSizeForTheUpdateField =
          this->m_TransformMethods[currentStageNumber].m_UpdateFieldMeshSizeAtBaseLevel;
        std::vector<unsigned int> meshSizeForTheTotalField =
          this->m_TransformMethods[currentStageNumber].m_TotalFieldMeshSizeAtBaseLevel;

        outputDisplacementFieldTransform->SetSplineOrder( this->m_TransformMethods[currentStageNumber].m_SplineOrder );

        if( meshSizeForTheUpdateField.size() != VImageDimension || meshSizeForTheTotalField.size() != VImageDimension )
          {
          ::ants::antscout << "ERROR:  The mesh size(s) don't match the ImageDimension." << std::endl;
          return EXIT_FAILURE;
          }

        typename BSplineDisplacementFieldTransformType::ArrayType updateMeshSize;
        typename BSplineDisplacementFieldTransformType::ArrayType totalMeshSize;
        for( unsigned int d = 0; d < VImageDimension; d++ )
          {
          updateMeshSize[d] = meshSizeForTheUpdateField[d];
          totalMeshSize[d] = meshSizeForTheTotalField[d];
          }
        // Create the transform adaptors specific to B-splines
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.

          typedef itk::ShrinkImageFilter<DisplacementFieldType, DisplacementFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( displacementField );
          shrinkFilter->Update();

          typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
            BSplineDisplacementFieldTransformType>
          BSplineDisplacementFieldTransformAdaptorType;
          typename BSplineDisplacementFieldTransformAdaptorType::Pointer bsplineFieldTransformAdaptor =
            BSplineDisplacementFieldTransformAdaptorType::New();
          bsplineFieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          bsplineFieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          bsplineFieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          bsplineFieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          bsplineFieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          // A good heuristic is to double the b-spline mesh resolution at each level
          typename BSplineDisplacementFieldTransformType::ArrayType newUpdateMeshSize = updateMeshSize;
          typename BSplineDisplacementFieldTransformType::ArrayType newTotalMeshSize = totalMeshSize;
          for( unsigned int d = 0; d < VImageDimension; d++ )
            {
            newUpdateMeshSize[d] = newUpdateMeshSize[d] << ( level + 1 );
            newTotalMeshSize[d] = newTotalMeshSize[d] << ( level + 1 );
            }
          bsplineFieldTransformAdaptor->SetMeshSizeForTheUpdateField( newUpdateMeshSize );
          bsplineFieldTransformAdaptor->SetMeshSizeForTheTotalField( newTotalMeshSize );

          adaptors.push_back( bsplineFieldTransformAdaptor.GetPointer() );
          }

        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename DisplacementFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        displacementFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        displacementFieldRegistration->SetOptimizer( optimizer );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running bspline displacement field registration (updateMeshSizeAtBaseLevel = "
                         << updateMeshSize << ", totalMeshSizeAtBaseLevel = " << totalMeshSize << ") ***" << std::endl
                         << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case BSpline:
        {
        const unsigned int SplineOrder = 3;
        typedef itk::BSplineTransform<RealType, VImageDimension, SplineOrder> BSplineTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType, BSplineTransformType> BSplineRegistrationType;
        typename BSplineRegistrationType::Pointer bsplineRegistration = BSplineRegistrationType::New();

        typename BSplineTransformType::Pointer outputBSplineTransform =
          const_cast<BSplineTransformType *>( bsplineRegistration->GetOutput()->Get() );

        const std::vector<unsigned int> & size =
          this->m_TransformMethods[currentStageNumber].m_MeshSizeAtBaseLevel;

        typename BSplineTransformType::PhysicalDimensionsType physicalDimensions;
        typename BSplineTransformType::MeshSizeType meshSize;
        for( unsigned int d = 0; d < VImageDimension; d++ )
          {
          physicalDimensions[d] = fixedImage->GetSpacing()[d]
            * static_cast<RealType>( fixedImage->GetLargestPossibleRegion().GetSize()[d] - 1 );
          meshSize[d] = size[d];
          }

        // Create the transform adaptors

        typedef itk::BSplineTransformParametersAdaptor<BSplineTransformType> BSplineTransformAdaptorType;
        typename BSplineRegistrationType::TransformParametersAdaptorsContainerType adaptors;
        // Create the transform adaptors specific to B-splines
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          typedef itk::ShrinkImageFilter<ImageType, ImageType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( fixedImage );
          shrinkFilter->Update();

          // A good heuristic is to double the b-spline mesh resolution at each level

          typename BSplineTransformType::MeshSizeType requiredMeshSize;
          for( unsigned int d = 0; d < VImageDimension; d++ )
            {
            requiredMeshSize[d] = meshSize[d] << level;
            }

          typedef itk::BSplineTransformParametersAdaptor<BSplineTransformType> BSplineAdaptorType;
          typename BSplineAdaptorType::Pointer bsplineAdaptor = BSplineAdaptorType::New();
          bsplineAdaptor->SetTransform( outputBSplineTransform );
          bsplineAdaptor->SetRequiredTransformDomainMeshSize( requiredMeshSize );
          bsplineAdaptor->SetRequiredTransformDomainOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          bsplineAdaptor->SetRequiredTransformDomainDirection( shrinkFilter->GetOutput()->GetDirection() );
          bsplineAdaptor->SetRequiredTransformDomainPhysicalDimensions( physicalDimensions );

          adaptors.push_back( bsplineAdaptor.GetPointer() );
          }

        bsplineRegistration->SetFixedImage( preprocessFixedImage );
        bsplineRegistration->SetMovingImage( preprocessMovingImage );
        bsplineRegistration->SetNumberOfLevels( numberOfLevels );
        bsplineRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        bsplineRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        bsplineRegistration->SetMetric( metric );
        bsplineRegistration->SetMetricSamplingStrategy(
          static_cast<typename BSplineRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        bsplineRegistration->SetMetricSamplingPercentage( samplingPercentage );
        bsplineRegistration->SetOptimizer( optimizer );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          bsplineRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          bsplineRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        bsplineRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );
        outputBSplineTransform->SetTransformDomainOrigin( fixedImage->GetOrigin() );
        outputBSplineTransform->SetTransformDomainPhysicalDimensions( physicalDimensions );
        outputBSplineTransform->SetTransformDomainMeshSize( meshSize );
        outputBSplineTransform->SetTransformDomainDirection( fixedImage->GetDirection() );
        outputBSplineTransform->SetIdentity();

        typedef antsRegistrationCommandIterationUpdate<BSplineRegistrationType> BSplineCommandType;
        typename BSplineCommandType::Pointer bsplineObserver = BSplineCommandType::New();
        bsplineObserver->SetLogStream(*this->m_LogStream);
        bsplineObserver->SetNumberOfIterations( currentStageIterations );

        bsplineRegistration->AddObserver( itk::IterationEvent(), bsplineObserver );
        bsplineRegistration->AddObserver( itk::InitializeEvent(), bsplineObserver );

        try
          {
          this->Logger() << std::endl << "*** Running bspline registration (meshSizeAtBaseLevel = " << meshSize
                         << ") ***"
                         << std::endl << std::endl;
          bsplineObserver->Execute( bsplineRegistration, itk::StartEvent() );
          bsplineRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }
        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputBSplineTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case TimeVaryingVelocityField:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );

        // Determine the parameters (size, spacing, etc) for the time-varying velocity field

        typedef itk::Image<VectorType, VImageDimension + 1> TimeVaryingVelocityFieldType;

        typename TimeVaryingVelocityFieldType::IndexType velocityFieldIndex;
        typename TimeVaryingVelocityFieldType::SizeType velocityFieldSize;
        typename TimeVaryingVelocityFieldType::PointType velocityFieldOrigin;
        typename TimeVaryingVelocityFieldType::SpacingType velocityFieldSpacing;
        typename TimeVaryingVelocityFieldType::DirectionType velocityFieldDirection;
        typename TimeVaryingVelocityFieldType::RegionType velocityFieldRegion;

        typename ImageType::IndexType fixedImageIndex = fixedImage->GetBufferedRegion().GetIndex();
        typename ImageType::SizeType fixedImageSize = fixedImage->GetBufferedRegion().GetSize();
        typename ImageType::PointType fixedImageOrigin = fixedImage->GetOrigin();
        typename ImageType::SpacingType fixedImageSpacing = fixedImage->GetSpacing();
        typename ImageType::DirectionType fixedImageDirection = fixedImage->GetDirection();

        unsigned int numberOfTimeIndices = this->m_TransformMethods[currentStageNumber].m_NumberOfTimeIndices;

        velocityFieldIndex.Fill( 0 );
        velocityFieldSize.Fill( numberOfTimeIndices );
        velocityFieldOrigin.Fill( 0.0 );
        velocityFieldSpacing.Fill( 1.0 );
        velocityFieldDirection.SetIdentity();
        for( unsigned int i = 0; i < VImageDimension; i++ )
          {
          velocityFieldIndex[i] = fixedImageIndex[i];
          velocityFieldSize[i] = fixedImageSize[i];
          velocityFieldOrigin[i] = fixedImageOrigin[i];
          velocityFieldSpacing[i] = fixedImageSpacing[i];
          for( unsigned int j = 0; j < VImageDimension; j++ )
            {
            velocityFieldDirection[i][j] = fixedImageDirection[i][j];
            }
          }

        velocityFieldRegion.SetSize( velocityFieldSize );
        velocityFieldRegion.SetIndex( velocityFieldIndex );
        typename TimeVaryingVelocityFieldType::Pointer velocityField =
          AllocImage<TimeVaryingVelocityFieldType>(velocityFieldRegion,
                                                   velocityFieldSpacing,
                                                   velocityFieldOrigin,
                                                   velocityFieldDirection,
                                                   zeroVector);

        // Extract parameters

        RealType varianceForUpdateField = this->m_TransformMethods[currentStageNumber].m_UpdateFieldVarianceInVarianceSpace;
        RealType varianceForUpdateFieldTime = this->m_TransformMethods[currentStageNumber].m_UpdateFieldTimeSigma;
        RealType varianceForTotalField = this->m_TransformMethods[currentStageNumber].m_TotalFieldVarianceInVarianceSpace;
        RealType varianceForTotalFieldTime = this->m_TransformMethods[currentStageNumber].m_TotalFieldTimeSigma;

        typedef itk::TimeVaryingVelocityFieldImageRegistrationMethodv4<ImageType, ImageType>
        VelocityFieldRegistrationType;
        typename VelocityFieldRegistrationType::Pointer velocityFieldRegistration = VelocityFieldRegistrationType::New();

        typedef typename VelocityFieldRegistrationType::OutputTransformType OutputTransformType;
        typename OutputTransformType::Pointer outputTransform =
          const_cast<OutputTransformType *>( velocityFieldRegistration->GetOutput()->Get() );

        velocityFieldRegistration->SetFixedImage( preprocessFixedImage );
        velocityFieldRegistration->SetMovingImage( preprocessMovingImage );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          velocityFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          velocityFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        velocityFieldRegistration->SetNumberOfLevels( numberOfLevels );
        velocityFieldRegistration->SetMetric( metric );
        velocityFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename VelocityFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        velocityFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        velocityFieldRegistration->SetLearningRate( learningRate );
        velocityFieldRegistration->SetConvergenceThreshold( convergenceThreshold );
        velocityFieldRegistration->SetConvergenceWindowSize( convergenceWindowSize );
        outputTransform->SetGaussianSpatialSmoothingVarianceForTheTotalField( varianceForTotalField );
        outputTransform->SetGaussianSpatialSmoothingVarianceForTheUpdateField( varianceForUpdateField );
        outputTransform->SetGaussianTemporalSmoothingVarianceForTheTotalField( varianceForTotalFieldTime );
        outputTransform->SetGaussianTemporalSmoothingVarianceForTheUpdateField( varianceForUpdateFieldTime );

        outputTransform->SetTimeVaryingVelocityField( velocityField );
        outputTransform->SetLowerTimeBound( 0.0 );
        outputTransform->SetUpperTimeBound( 1.0 );

        typename VelocityFieldRegistrationType::NumberOfIterationsArrayType numberOfIterationsPerLevel;
        numberOfIterationsPerLevel.SetSize( numberOfLevels );
        for( unsigned int d = 0; d < numberOfLevels; d++ )
          {
          numberOfIterationsPerLevel[d] = currentStageIterations[d];
          }
        velocityFieldRegistration->SetNumberOfIterationsPerLevel( numberOfIterationsPerLevel );
        velocityFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        velocityFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );

        typedef itk::TimeVaryingVelocityFieldTransformParametersAdaptor<OutputTransformType>
        VelocityFieldTransformAdaptorType;

        typename VelocityFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;
        for( unsigned int level = 0; level < shrinkFactorsPerLevel.Size(); level++ )
          {
          typedef itk::ShrinkImageFilter<ImageType, ImageType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( fixedImage );
          shrinkFilter->Update();

          // Although we shrink the images for the given levels,
          // we keep the size in time the same

          velocityFieldSize.Fill( numberOfTimeIndices );
          velocityFieldOrigin.Fill( 0.0 );
          velocityFieldSpacing.Fill( 1.0 );
          velocityFieldDirection.SetIdentity();

          fixedImageSize = shrinkFilter->GetOutput()->GetBufferedRegion().GetSize();
          fixedImageOrigin = shrinkFilter->GetOutput()->GetOrigin();
          fixedImageSpacing = shrinkFilter->GetOutput()->GetSpacing();
          fixedImageDirection = shrinkFilter->GetOutput()->GetDirection();
          for( unsigned int i = 0; i < VImageDimension; i++ )
            {
            velocityFieldSize[i] = fixedImageSize[i];
            velocityFieldOrigin[i] = fixedImageOrigin[i];
            velocityFieldSpacing[i] = fixedImageSpacing[i];
            for( unsigned int j = 0; j < VImageDimension; j++ )
              {
              velocityFieldDirection[i][j] = fixedImageDirection[i][j];
              }
            }

          typename VelocityFieldTransformAdaptorType::Pointer fieldTransformAdaptor =
            VelocityFieldTransformAdaptorType::New();
          fieldTransformAdaptor->SetRequiredSpacing( velocityFieldSpacing );
          fieldTransformAdaptor->SetRequiredSize( velocityFieldSize );
          fieldTransformAdaptor->SetRequiredDirection( velocityFieldDirection );
          fieldTransformAdaptor->SetRequiredOrigin( velocityFieldOrigin );

          adaptors.push_back( fieldTransformAdaptor.GetPointer() );
          }

        velocityFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );

        typedef antsRegistrationCommandIterationUpdate<VelocityFieldRegistrationType> VelocityFieldCommandType;
        typename VelocityFieldCommandType::Pointer velocityFieldRegistrationObserver = VelocityFieldCommandType::New();
        velocityFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        velocityFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        velocityFieldRegistration->AddObserver( itk::IterationEvent(), velocityFieldRegistrationObserver );
        velocityFieldRegistration->AddObserver( itk::InitializeEvent(), velocityFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running time-varying velocity field registration (varianceForUpdateField = "
                         << varianceForUpdateField << ", varianceForTotalField = " << varianceForTotalField
                         << ", varianceForUpdateFieldTime = "
                         << varianceForUpdateFieldTime << ", varianceForTotalFieldTime = " << varianceForTotalFieldTime
                         << ") ***" << std::endl << std::endl;
          velocityFieldRegistrationObserver->Execute( velocityFieldRegistration, itk::StartEvent() );
          velocityFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }
        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case TimeVaryingBSplineVelocityField:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );

        // Determine the parameters (size, spacing, etc) for the time-varying velocity field control point lattice

        const std::vector<unsigned int> & meshSize = this->m_TransformMethods[currentStageNumber].m_VelocityFieldMeshSize;
        if( meshSize.size() != VImageDimension + 1 )
          {
          ::ants::antscout << "The transform domain mesh size does not have the correct number of elements."
                           << "For image dimension = " << VImageDimension << ", you need " << VImageDimension + 1
                           << "elements. " << std::endl;
          return EXIT_FAILURE;
          }

        unsigned int numberOfTimePointSamples =  this->m_TransformMethods[currentStageNumber].m_NumberOfTimePointSamples;
        unsigned int splineOrder = this->m_TransformMethods[currentStageNumber].m_SplineOrder;

        typedef itk::Image<VectorType, VImageDimension + 1> TimeVaryingVelocityFieldControlPointLatticeType;

        typename ImageType::SizeType fixedImageSize = fixedImage->GetBufferedRegion().GetSize();
        typename ImageType::PointType fixedImageOrigin = fixedImage->GetOrigin();
        typename ImageType::SpacingType fixedImageSpacing = fixedImage->GetSpacing();
        typename ImageType::DirectionType fixedImageDirection = fixedImage->GetDirection();

        typename TimeVaryingVelocityFieldControlPointLatticeType::SizeType transformDomainMeshSize;
        typename TimeVaryingVelocityFieldControlPointLatticeType::PointType transformDomainOrigin;
        typename TimeVaryingVelocityFieldControlPointLatticeType::SpacingType transformDomainPhysicalDimensions;
        typename TimeVaryingVelocityFieldControlPointLatticeType::DirectionType transformDomainDirection;

        transformDomainDirection.SetIdentity();
        transformDomainOrigin.Fill( 0.0 );
        transformDomainPhysicalDimensions.Fill( 1.0 );
        for( unsigned int i = 0; i < VImageDimension; i++ )
          {
          transformDomainOrigin[i] = fixedImageOrigin[i];
          transformDomainMeshSize[i] = 3;
          transformDomainPhysicalDimensions[i] = static_cast<double>( fixedImageSize[i] - 1 ) * fixedImageSpacing[i];
          for( unsigned int j = 0; j < VImageDimension; j++ )
            {
            transformDomainDirection[i][j] = fixedImageDirection[i][j];
            }
          }
        for( unsigned int i = 0; i < meshSize.size(); i++ )
          {
          transformDomainMeshSize[i] = meshSize[i];
          }
        typename TimeVaryingVelocityFieldControlPointLatticeType::SizeType initialTransformDomainMeshSize =
          transformDomainMeshSize;

        typedef itk::TimeVaryingBSplineVelocityFieldImageRegistrationMethod<ImageType, ImageType>
        VelocityFieldRegistrationType;
        typename VelocityFieldRegistrationType::Pointer velocityFieldRegistration = VelocityFieldRegistrationType::New();

        typedef typename VelocityFieldRegistrationType::OutputTransformType OutputTransformType;
        typename OutputTransformType::Pointer outputTransform =
          const_cast<OutputTransformType *>( velocityFieldRegistration->GetOutput()->Get() );

        velocityFieldRegistration->SetFixedImage( preprocessFixedImage );
        velocityFieldRegistration->SetMovingImage( preprocessMovingImage );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          velocityFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          velocityFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        velocityFieldRegistration->SetNumberOfLevels( numberOfLevels );
        velocityFieldRegistration->SetNumberOfTimePointSamples( numberOfTimePointSamples );
        velocityFieldRegistration->SetMetric( metric );
        velocityFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename VelocityFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        velocityFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        velocityFieldRegistration->SetLearningRate( learningRate );
        velocityFieldRegistration->SetConvergenceThreshold( convergenceThreshold );
        velocityFieldRegistration->SetConvergenceWindowSize( convergenceWindowSize );
        outputTransform->SetSplineOrder( splineOrder );
        outputTransform->SetLowerTimeBound( 0.0 );
        outputTransform->SetUpperTimeBound( 1.0 );

        typedef itk::TimeVaryingBSplineVelocityFieldTransformParametersAdaptor<OutputTransformType>
        VelocityFieldTransformAdaptorType;
        typename VelocityFieldTransformAdaptorType::Pointer initialFieldTransformAdaptor =
          VelocityFieldTransformAdaptorType::New();
        initialFieldTransformAdaptor->SetTransform( outputTransform );
        initialFieldTransformAdaptor->SetRequiredTransformDomainOrigin( transformDomainOrigin );
        initialFieldTransformAdaptor->SetRequiredTransformDomainPhysicalDimensions( transformDomainPhysicalDimensions );
        initialFieldTransformAdaptor->SetRequiredTransformDomainMeshSize( transformDomainMeshSize );
        initialFieldTransformAdaptor->SetRequiredTransformDomainDirection( transformDomainDirection );

        typename TimeVaryingVelocityFieldControlPointLatticeType::Pointer
          velocityFieldLattice =
          AllocImage<TimeVaryingVelocityFieldControlPointLatticeType>
          (initialFieldTransformAdaptor->GetRequiredControlPointLatticeSize(),
           initialFieldTransformAdaptor->GetRequiredControlPointLatticeSpacing(),
           initialFieldTransformAdaptor->GetRequiredControlPointLatticeOrigin(),
           initialFieldTransformAdaptor->GetRequiredControlPointLatticeDirection(),
           zeroVector);

        typename OutputTransformType::VelocityFieldPointType        sampledVelocityFieldOrigin;
        typename OutputTransformType::VelocityFieldSpacingType      sampledVelocityFieldSpacing;
        typename OutputTransformType::VelocityFieldSizeType         sampledVelocityFieldSize;
        typename OutputTransformType::VelocityFieldDirectionType    sampledVelocityFieldDirection;

        sampledVelocityFieldOrigin.Fill( 0.0 );
        sampledVelocityFieldSpacing.Fill( 1.0 );
        sampledVelocityFieldSize.Fill( numberOfTimePointSamples );
        sampledVelocityFieldDirection.SetIdentity();
        for( unsigned int i = 0; i < VImageDimension; i++ )
          {
          sampledVelocityFieldOrigin[i] = fixedImage->GetOrigin()[i];
          sampledVelocityFieldSpacing[i] = fixedImage->GetSpacing()[i];
          sampledVelocityFieldSize[i] = fixedImage->GetRequestedRegion().GetSize()[i];
          for( unsigned int j = 0; j < VImageDimension; j++ )
            {
            sampledVelocityFieldDirection[i][j] = fixedImage->GetDirection()[i][j];
            }
          }

        outputTransform->SetTimeVaryingVelocityFieldControlPointLattice( velocityFieldLattice );
        outputTransform->SetVelocityFieldOrigin( sampledVelocityFieldOrigin );
        outputTransform->SetVelocityFieldDirection( sampledVelocityFieldDirection );
        outputTransform->SetVelocityFieldSpacing( sampledVelocityFieldSpacing );
        outputTransform->SetVelocityFieldSize( sampledVelocityFieldSize );

        typename VelocityFieldRegistrationType::NumberOfIterationsArrayType numberOfIterationsPerLevel;
        numberOfIterationsPerLevel.SetSize( numberOfLevels );
        for( unsigned int d = 0; d < numberOfLevels; d++ )
          {
          numberOfIterationsPerLevel[d] = currentStageIterations[d];
          }
        velocityFieldRegistration->SetNumberOfIterationsPerLevel( numberOfIterationsPerLevel );
        velocityFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        velocityFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );

        typename VelocityFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;
        for( unsigned int level = 0; level < shrinkFactorsPerLevel.Size(); level++ )
          {
          typename VelocityFieldTransformAdaptorType::Pointer fieldTransformAdaptor =
            VelocityFieldTransformAdaptorType::New();
          fieldTransformAdaptor->SetTransform( outputTransform );
          fieldTransformAdaptor->SetRequiredTransformDomainOrigin( transformDomainOrigin );
          fieldTransformAdaptor->SetRequiredTransformDomainMeshSize( transformDomainMeshSize );
          fieldTransformAdaptor->SetRequiredTransformDomainDirection( transformDomainDirection );
          fieldTransformAdaptor->SetRequiredTransformDomainPhysicalDimensions( transformDomainPhysicalDimensions );

          adaptors.push_back( fieldTransformAdaptor.GetPointer() );
          for( unsigned int i = 0; i <= VImageDimension; i++ )
            {
            transformDomainMeshSize[i] <<= 1;
            }

          }
        velocityFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );

        typedef antsRegistrationCommandIterationUpdate<VelocityFieldRegistrationType> VelocityFieldCommandType;
        typename VelocityFieldCommandType::Pointer velocityFieldRegistrationObserver = VelocityFieldCommandType::New();
        velocityFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        velocityFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        velocityFieldRegistration->AddObserver( itk::IterationEvent(), velocityFieldRegistrationObserver );
        velocityFieldRegistration->AddObserver( itk::InitializeEvent(), velocityFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running time-varying b-spline velocity field registration (initial mesh size = "
                         << initialTransformDomainMeshSize << ") ***" << std::endl << std::endl;
          velocityFieldRegistrationObserver->Execute( velocityFieldRegistration, itk::StartEvent() );
          velocityFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }
        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case SyN:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );
        typedef itk::Image<VectorType, VImageDimension> DisplacementFieldType;
        typename DisplacementFieldType::Pointer displacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typename DisplacementFieldType::Pointer inverseDisplacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typedef itk::SyNImageRegistrationMethod<ImageType, ImageType,
                                                DisplacementFieldTransformType> DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename DisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<DisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );

        // Create the transform adaptors

        typedef itk::DisplacementFieldTransformParametersAdaptor<DisplacementFieldTransformType>
        DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;
        // Create the transform adaptors
        // For the gaussian displacement field, the specified variances are in image spacing terms
        // and, in normal practice, we typically don't change these values at each level.  However,
        // if the user wishes to add that option, they can use the class
        // GaussianSmoothingOnUpdateDisplacementFieldTransformAdaptor
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // TODO:
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.
          typedef itk::ShrinkImageFilter<DisplacementFieldType, DisplacementFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( displacementField );
          shrinkFilter->Update();

          typename DisplacementFieldTransformAdaptorType::Pointer fieldTransformAdaptor =
            DisplacementFieldTransformAdaptorType::New();
          fieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          fieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          fieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          fieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          fieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          adaptors.push_back( fieldTransformAdaptor.GetPointer() );
          }

        // Extract parameters
        typename DisplacementFieldRegistrationType::NumberOfIterationsArrayType numberOfIterationsPerLevel;
        numberOfIterationsPerLevel.SetSize( numberOfLevels );
        for( unsigned int d = 0; d < numberOfLevels; d++ )
          {
          numberOfIterationsPerLevel[d] = currentStageIterations[d];
          }

        const RealType varianceForUpdateField = this->m_TransformMethods[currentStageNumber].m_UpdateFieldVarianceInVarianceSpace;
        const RealType varianceForTotalField = this->m_TransformMethods[currentStageNumber].m_TotalFieldVarianceInVarianceSpace;

        displacementFieldRegistration->SetDownsampleImagesForMetricDerivatives( true );
        displacementFieldRegistration->SetAverageMidPointGradients( false );
        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetLearningRate( learningRate );
        displacementFieldRegistration->SetConvergenceThreshold( convergenceThreshold );
        displacementFieldRegistration->SetConvergenceWindowSize( convergenceWindowSize );
        displacementFieldRegistration->SetNumberOfIterationsPerLevel( numberOfIterationsPerLevel );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );
        displacementFieldRegistration->SetGaussianSmoothingVarianceForTheUpdateField( varianceForUpdateField );
        displacementFieldRegistration->SetGaussianSmoothingVarianceForTheTotalField( varianceForTotalField );
        outputDisplacementFieldTransform->SetDisplacementField( displacementField );
        outputDisplacementFieldTransform->SetInverseDisplacementField( inverseDisplacementField );

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl << "*** Running SyN registration (varianceForUpdateField = "
                         << varianceForUpdateField << ", varianceForTotalField = " << varianceForTotalField << ") ***"
                         << std::endl << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );
        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case BSplineSyN:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );
        typedef itk::Image<VectorType, VImageDimension> DisplacementFieldType;
        typename DisplacementFieldType::Pointer displacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typename DisplacementFieldType::Pointer inverseDisplacementField =
          AllocImage<DisplacementFieldType>(fixedImage,zeroVector);

        typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransform<RealType,
                                                                        VImageDimension>
        BSplineDisplacementFieldTransformType;

        typedef itk::BSplineSyNImageRegistrationMethod<ImageType, ImageType,
                                                       BSplineDisplacementFieldTransformType>
        DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename BSplineDisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<BSplineDisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );

        // Create the transform adaptors

        typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
          BSplineDisplacementFieldTransformType>
        DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;

        // Extract parameters

        const std::vector<unsigned int> & meshSizeForTheUpdateField =
          this->m_TransformMethods[currentStageNumber].m_UpdateFieldMeshSizeAtBaseLevel;
        std::vector<unsigned int> meshSizeForTheTotalField =
          this->m_TransformMethods[currentStageNumber].m_TotalFieldMeshSizeAtBaseLevel;

        outputDisplacementFieldTransform->SetSplineOrder( this->m_TransformMethods[currentStageNumber].m_SplineOrder );

        if( meshSizeForTheUpdateField.size() != VImageDimension || meshSizeForTheTotalField.size() != VImageDimension )
          {
          ::ants::antscout << "ERROR:  The mesh size(s) don't match the ImageDimension." << std::endl;
          return EXIT_FAILURE;
          }

        typename BSplineDisplacementFieldTransformType::ArrayType updateMeshSize;
        typename BSplineDisplacementFieldTransformType::ArrayType totalMeshSize;
        for( unsigned int d = 0; d < VImageDimension; d++ )
          {
          updateMeshSize[d] = meshSizeForTheUpdateField[d];
          totalMeshSize[d] = meshSizeForTheTotalField[d];
          }
        // Create the transform adaptors
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.

          typedef itk::ShrinkImageFilter<DisplacementFieldType, DisplacementFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( displacementField );
          shrinkFilter->Update();

          typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
            BSplineDisplacementFieldTransformType>
          BSplineDisplacementFieldTransformAdaptorType;
          typename BSplineDisplacementFieldTransformAdaptorType::Pointer bsplineFieldTransformAdaptor =
            BSplineDisplacementFieldTransformAdaptorType::New();
          bsplineFieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          bsplineFieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          bsplineFieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          bsplineFieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          bsplineFieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          // A good heuristic is to double the b-spline mesh resolution at each level
          typename BSplineDisplacementFieldTransformType::ArrayType newUpdateMeshSize = updateMeshSize;
          typename BSplineDisplacementFieldTransformType::ArrayType newTotalMeshSize = totalMeshSize;
          for( unsigned int d = 0; d < VImageDimension; d++ )
            {
            newUpdateMeshSize[d] = newUpdateMeshSize[d] << ( level );
            newTotalMeshSize[d] = newTotalMeshSize[d] << ( level );
            }

          bsplineFieldTransformAdaptor->SetMeshSizeForTheUpdateField( newUpdateMeshSize );
          bsplineFieldTransformAdaptor->SetMeshSizeForTheTotalField( newTotalMeshSize );

          adaptors.push_back( bsplineFieldTransformAdaptor.GetPointer() );
          }

        // Extract parameters
        typename DisplacementFieldRegistrationType::NumberOfIterationsArrayType numberOfIterationsPerLevel;
        numberOfIterationsPerLevel.SetSize( numberOfLevels );
        for( unsigned int d = 0; d < numberOfLevels; d++ )
          {
          numberOfIterationsPerLevel[d] = currentStageIterations[d];
          }

        displacementFieldRegistration->SetDownsampleImagesForMetricDerivatives( true );
        displacementFieldRegistration->SetAverageMidPointGradients( false );
        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetLearningRate( learningRate );
        displacementFieldRegistration->SetConvergenceThreshold( convergenceThreshold );
        displacementFieldRegistration->SetConvergenceWindowSize( convergenceWindowSize );
        displacementFieldRegistration->SetNumberOfIterationsPerLevel( numberOfIterationsPerLevel );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );
        outputDisplacementFieldTransform->SetDisplacementField( displacementField );
        outputDisplacementFieldTransform->SetInverseDisplacementField( inverseDisplacementField );

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl << "*** Running B-spline SyN registration (updateMeshSizeAtBaseLevel = "
                         << updateMeshSize << ", totalMeshSizeAtBaseLevel = " << totalMeshSize << ") ***" << std::endl
                         << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case Exponential:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );

        typedef itk::Image<VectorType, VImageDimension> ConstantVelocityFieldType;

        typename ConstantVelocityFieldType::Pointer constantVelocityField =
          AllocImage<ConstantVelocityFieldType>( fixedImage, zeroVector );

        typedef itk::GaussianExponentialDiffeomorphicTransform<RealType, VImageDimension> GaussianDisplacementFieldTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               GaussianDisplacementFieldTransformType>
        DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename GaussianDisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<GaussianDisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );

        // Create the transform adaptors

        typedef itk::GaussianExponentialDiffeomorphicTransformParametersAdaptor<GaussianDisplacementFieldTransformType>
          DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;

        // Extract parameters
        RealType varianceForUpdateField = this->m_TransformMethods[currentStageNumber].m_UpdateFieldVarianceInVarianceSpace;
        RealType varianceForVelocityField  = this->m_TransformMethods[currentStageNumber].m_VelocityFieldVarianceInVarianceSpace;
        unsigned int numberOfIntegrationSteps = this->m_TransformMethods[currentStageNumber].m_NumberOfTimeIndices;

        outputDisplacementFieldTransform->SetGaussianSmoothingVarianceForTheUpdateField( varianceForUpdateField );
        outputDisplacementFieldTransform->SetGaussianSmoothingVarianceForTheConstantVelocityField( varianceForVelocityField );
        if( numberOfIntegrationSteps == 0 )
          {
          outputDisplacementFieldTransform->SetCalculateNumberOfIntegrationStepsAutomatically( true );
          }
        else
          {
          outputDisplacementFieldTransform->SetNumberOfIntegrationSteps( numberOfIntegrationSteps );
          }
        outputDisplacementFieldTransform->SetConstantVelocityField( constantVelocityField );
        // Create the transform adaptors
        // For the gaussian displacement field, the specified variances are in image spacing terms
        // and, in normal practice, we typically don't change these values at each level.  However,
        // if the user wishes to add that option, they can use the class
        // GaussianSmoothingOnUpdateDisplacementFieldTransformAdaptor
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.

          typedef itk::ShrinkImageFilter<ConstantVelocityFieldType, ConstantVelocityFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( constantVelocityField );
          shrinkFilter->Update();

          typename DisplacementFieldTransformAdaptorType::Pointer fieldTransformAdaptor =
            DisplacementFieldTransformAdaptorType::New();
          fieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          fieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          fieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          fieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          fieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          fieldTransformAdaptor->SetGaussianSmoothingVarianceForTheUpdateField( varianceForUpdateField );
          fieldTransformAdaptor->SetGaussianSmoothingVarianceForTheConstantVelocityField( varianceForVelocityField );

          adaptors.push_back( fieldTransformAdaptor.GetPointer() );
          }

        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename DisplacementFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        displacementFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        displacementFieldRegistration->SetOptimizer( optimizer2 );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream );
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running gaussian exponential field registration (varianceForUpdateField = "
                         << varianceForUpdateField << ", varianceForVelocityField = " << varianceForVelocityField << ") ***"
                         << std::endl << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      case BSplineExponential:
        {
        typedef itk::Vector<RealType, VImageDimension> VectorType;
        VectorType zeroVector( 0.0 );
        typedef itk::Image<VectorType, VImageDimension> ConstantVelocityFieldType;

        typename ConstantVelocityFieldType::Pointer constantVelocityField =
          AllocImage<ConstantVelocityFieldType>( fixedImage, zeroVector );

        typedef itk::BSplineExponentialDiffeomorphicTransform<RealType, VImageDimension> BSplineDisplacementFieldTransformType;

        typedef itk::ImageRegistrationMethodv4<ImageType, ImageType,
                                               BSplineDisplacementFieldTransformType> DisplacementFieldRegistrationType;
        typename DisplacementFieldRegistrationType::Pointer displacementFieldRegistration =
          DisplacementFieldRegistrationType::New();

        typename BSplineDisplacementFieldTransformType::Pointer outputDisplacementFieldTransform =
          const_cast<BSplineDisplacementFieldTransformType *>( displacementFieldRegistration->GetOutput()->Get() );

        // Create the transform adaptors

        typedef itk::BSplineSmoothingOnUpdateDisplacementFieldTransformParametersAdaptor<
          BSplineDisplacementFieldTransformType>
        DisplacementFieldTransformAdaptorType;
        typename DisplacementFieldRegistrationType::TransformParametersAdaptorsContainerType adaptors;

        // Extract parameters

        const std::vector<unsigned int> & meshSizeForTheUpdateField =
          this->m_TransformMethods[currentStageNumber].m_UpdateFieldMeshSizeAtBaseLevel;
        std::vector<unsigned int> meshSizeForTheVelocityField =
          this->m_TransformMethods[currentStageNumber].m_VelocityFieldMeshSizeAtBaseLevel;
        unsigned int numberOfIntegrationSteps = this->m_TransformMethods[currentStageNumber].m_NumberOfTimeIndices;

        if( numberOfIntegrationSteps == 0 )
          {
          outputDisplacementFieldTransform->SetCalculateNumberOfIntegrationStepsAutomatically( true );
          }
        else
          {
          outputDisplacementFieldTransform->SetNumberOfIntegrationSteps( numberOfIntegrationSteps );
          }
        outputDisplacementFieldTransform->SetSplineOrder( this->m_TransformMethods[currentStageNumber].m_SplineOrder );
        outputDisplacementFieldTransform->SetConstantVelocityField( constantVelocityField );

        if( meshSizeForTheUpdateField.size() != VImageDimension || meshSizeForTheVelocityField.size() != VImageDimension )
          {
          ::ants::antscout << "ERROR:  The mesh size(s) don't match the ImageDimension." << std::endl;
          return EXIT_FAILURE;
          }

        typename BSplineDisplacementFieldTransformType::ArrayType updateMeshSize;
        typename BSplineDisplacementFieldTransformType::ArrayType velocityMeshSize;
        for( unsigned int d = 0; d < VImageDimension; d++ )
          {
          updateMeshSize[d] = meshSizeForTheUpdateField[d];
          velocityMeshSize[d] = meshSizeForTheVelocityField[d];
          }
        // Create the transform adaptors specific to B-splines
        for( unsigned int level = 0; level < numberOfLevels; level++ )
          {
          // We use the shrink image filter to calculate the fixed parameters of the virtual
          // domain at each level.  To speed up calculation and avoid unnecessary memory
          // usage, we could calculate these fixed parameters directly.

          typedef itk::ShrinkImageFilter<ConstantVelocityFieldType, ConstantVelocityFieldType> ShrinkFilterType;
          typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
          shrinkFilter->SetShrinkFactors( shrinkFactorsPerLevel[level] );
          shrinkFilter->SetInput( constantVelocityField );
          shrinkFilter->Update();

          typedef itk::BSplineExponentialDiffeomorphicTransformParametersAdaptor<BSplineDisplacementFieldTransformType>
            BSplineDisplacementFieldTransformAdaptorType;
          typename BSplineDisplacementFieldTransformAdaptorType::Pointer bsplineFieldTransformAdaptor =
            BSplineDisplacementFieldTransformAdaptorType::New();
          bsplineFieldTransformAdaptor->SetRequiredSpacing( shrinkFilter->GetOutput()->GetSpacing() );
          bsplineFieldTransformAdaptor->SetRequiredSize( shrinkFilter->GetOutput()->GetBufferedRegion().GetSize() );
          bsplineFieldTransformAdaptor->SetRequiredDirection( shrinkFilter->GetOutput()->GetDirection() );
          bsplineFieldTransformAdaptor->SetRequiredOrigin( shrinkFilter->GetOutput()->GetOrigin() );
          bsplineFieldTransformAdaptor->SetTransform( outputDisplacementFieldTransform );

          // A good heuristic is to double the b-spline mesh resolution at each level
          typename BSplineDisplacementFieldTransformType::ArrayType newUpdateMeshSize = updateMeshSize;
          typename BSplineDisplacementFieldTransformType::ArrayType newVelocityMeshSize = velocityMeshSize;
          for( unsigned int d = 0; d < VImageDimension; d++ )
            {
            newUpdateMeshSize[d] = newUpdateMeshSize[d] << ( level + 1 );
            newVelocityMeshSize[d] = newVelocityMeshSize[d] << ( level + 1 );
            }
          bsplineFieldTransformAdaptor->SetMeshSizeForTheUpdateField( newUpdateMeshSize );
          bsplineFieldTransformAdaptor->SetMeshSizeForTheConstantVelocityField( newVelocityMeshSize );

          adaptors.push_back( bsplineFieldTransformAdaptor.GetPointer() );
          }

        displacementFieldRegistration->SetFixedImage( preprocessFixedImage );
        displacementFieldRegistration->SetMovingImage( preprocessMovingImage );
        displacementFieldRegistration->SetNumberOfLevels( numberOfLevels );
        displacementFieldRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
        displacementFieldRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
        if( this->m_CompositeTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetMovingInitialTransform( this->m_CompositeTransform );
          }
        if( this->m_FixedInitialTransform->GetNumberOfTransforms() > 0 )
          {
          displacementFieldRegistration->SetFixedInitialTransform( this->m_FixedInitialTransform );
          }
        displacementFieldRegistration->SetMetric( metric );
        displacementFieldRegistration->SetMetricSamplingStrategy(
          static_cast<typename DisplacementFieldRegistrationType::MetricSamplingStrategyType>( metricSamplingStrategy ) );
        displacementFieldRegistration->SetMetricSamplingPercentage( samplingPercentage );
        displacementFieldRegistration->SetOptimizer( optimizer2 );
        displacementFieldRegistration->SetTransformParametersAdaptorsPerLevel( adaptors );

        typedef antsRegistrationCommandIterationUpdate<DisplacementFieldRegistrationType> DisplacementFieldCommandType;
        typename DisplacementFieldCommandType::Pointer displacementFieldRegistrationObserver =
          DisplacementFieldCommandType::New();
        displacementFieldRegistrationObserver->SetLogStream(*this->m_LogStream);
        displacementFieldRegistrationObserver->SetNumberOfIterations( currentStageIterations );

        displacementFieldRegistration->AddObserver( itk::IterationEvent(), displacementFieldRegistrationObserver );
        displacementFieldRegistration->AddObserver( itk::InitializeEvent(), displacementFieldRegistrationObserver );

        try
          {
          this->Logger() << std::endl
                         << "*** Running bspline exponential field registration (updateMeshSizeAtBaseLevel = "
                         << updateMeshSize << ", velocityMeshSizeAtBaseLevel = " << velocityMeshSize << ") ***" << std::endl
                         << std::endl;
          displacementFieldRegistrationObserver->Execute( displacementFieldRegistration, itk::StartEvent() );
          displacementFieldRegistration->Update();
          }
        catch( itk::ExceptionObject & e )
          {
          ::ants::antscout << "Exception caught: " << e << std::endl;
          return EXIT_FAILURE;
          }

        // Add calculated transform to the composite transform
        this->m_CompositeTransform->AddTransform( outputDisplacementFieldTransform );

        this->m_AllPreviousTransformsAreLinear = false;
        }
        break;
      default:
        ::ants::antscout << "ERROR:  Unrecognized transform option - " << whichTransform << std::endl;
        return EXIT_FAILURE;
      }
    //TODO:  Collapse Previous 2 Composite Transforms.  If the two previous composite transforms are
    //       compatible, then merge them into one transform
    //       Given that if the request is for
    //       Translation->Rigid->Similarity->Affine->MSE->SyN
    //       |------------------------------------|  |------|
    //       Affine                                  Warp
    //       The result should be a composite transform with only two elmeents [Affine, Warp]
    timer.Stop();
    this->Logger() << "  Elapsed time (stage " << currentStageNumber << "): " << timer.GetMean() << std::endl << std::endl;
    }

  if( this->m_ApplyLinearTransformsToFixedImageHeader &&
      this->m_CompositeLinearTransformForFixedImageHeader->GetNumberOfTransforms() > 0 )
    {
    this->m_CompositeTransform->PrependTransform( this->m_CompositeLinearTransformForFixedImageHeader );
    this->m_CompositeTransform->FlattenTransformQueue();
    }

  totalTimer.Stop();
  this->Logger() << std::endl << "Total elapsed time: " << totalTimer.GetMean() << std::endl;
  return EXIT_SUCCESS;
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetMovingInitialTransform( const TransformType *initialTransform )
{
  // Since the initial transform might be linear (or a composition of
  // linear transforms), we might want to add those initial transforms
  // to the moving image header for faster processing.

  typename CompositeTransformType::Pointer compToAdd;

  typename CompositeTransformType::ConstPointer compXfrm =
    dynamic_cast<const CompositeTransformType *>( initialTransform );
  if( compXfrm.IsNotNull() )
    {
    compToAdd = compXfrm->Clone();
    this->m_CompositeTransform = compToAdd;
    }
  else
    {
    compToAdd = CompositeTransformType::New();
    typename TransformType::Pointer xfrm = initialTransform->Clone();
    compToAdd->AddTransform( xfrm );
    this->m_CompositeTransform = compToAdd;
    }
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::SetFixedInitialTransform( const TransformType *initialTransform  )
{
  typename CompositeTransformType::Pointer compToAdd;

  typename CompositeTransformType::ConstPointer compXfrm =
    dynamic_cast<const CompositeTransformType *>( initialTransform );
  if( compXfrm.IsNotNull() )
    {
    compToAdd = compXfrm->Clone();

    if( this->m_ApplyLinearTransformsToFixedImageHeader && compXfrm->IsLinear() )
      {
      this->m_CompositeLinearTransformForFixedImageHeader = compToAdd;
      }
    else
      {
      this->m_FixedInitialTransform = compToAdd;
      this->m_AllPreviousTransformsAreLinear = false;
      }
    }
  else
    {
    compToAdd = CompositeTransformType::New();
    typename TransformType::Pointer xfrm = initialTransform->Clone();
    compToAdd->AddTransform( xfrm );
    if( this->m_ApplyLinearTransformsToFixedImageHeader && initialTransform->IsLinear() )
      {
      this->m_CompositeLinearTransformForFixedImageHeader = compToAdd;
      }
    else
      {
      this->m_FixedInitialTransform = compToAdd;
      this->m_AllPreviousTransformsAreLinear = false;
      }
    }
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::ApplyCompositeLinearTransformToImageHeader( const CompositeTransformType * compositeTransform, ImageBaseType * const image, const bool applyInverse )
{
  if( !compositeTransform->IsLinear() )
    {
    itkExceptionMacro( "The composite transform is not linear.  Cannot collapse it to the image header." );
    }

  typedef itk::AffineTransform<RealType, VImageDimension>      AffineTransformType;
  typedef typename AffineTransformType::Superclass             MatrixOffsetTransformBaseType;
  typedef itk::TranslationTransform<RealType, VImageDimension> TranslationTransformType;

  typename MatrixOffsetTransformBaseType::Pointer totalTransform = MatrixOffsetTransformBaseType::New();
  for( unsigned int n = 0; n < compositeTransform->GetNumberOfTransforms(); n++ )
    {
    typename TransformType::Pointer transform = compositeTransform->GetNthTransform( n );

    typename MatrixOffsetTransformBaseType::Pointer nthTransform = MatrixOffsetTransformBaseType::New();

    typename TranslationTransformType::Pointer translationTransform =
      dynamic_cast<TranslationTransformType *>( transform.GetPointer() );
    if( translationTransform.IsNotNull() )
      {
      nthTransform->SetOffset( translationTransform->GetOffset() );
      }
    else
      {
      typename MatrixOffsetTransformBaseType::Pointer matrixOffsetTransform =
        dynamic_cast<MatrixOffsetTransformBaseType *>( transform.GetPointer() );
      nthTransform->SetMatrix( matrixOffsetTransform->GetMatrix() );
      nthTransform->SetOffset( matrixOffsetTransform->GetOffset() );
      }
    totalTransform->Compose( nthTransform, false );
    }

  typename ImageType::PointType origin = image->GetOrigin();
  typename ImageType::DirectionType direction = image->GetDirection();

  typename MatrixOffsetTransformBaseType::Pointer imageTransform = MatrixOffsetTransformBaseType::New();
  imageTransform->SetMatrix( direction );
  imageTransform->SetOffset( origin.GetVectorFromOrigin() );

  if( applyInverse )
    {
    typename MatrixOffsetTransformBaseType::Pointer inverseImageTransform = MatrixOffsetTransformBaseType::New();
    inverseImageTransform->SetMatrix( dynamic_cast<MatrixOffsetTransformBaseType *>( imageTransform->GetInverseTransform().GetPointer() )->GetMatrix() );
    inverseImageTransform->SetOffset( -( inverseImageTransform->GetMatrix() * imageTransform->GetOffset() ) );

    totalTransform->Compose( inverseImageTransform, false );

    typename MatrixOffsetTransformBaseType::MatrixType inverseMatrix = dynamic_cast<MatrixOffsetTransformBaseType *>( totalTransform->GetInverseTransform().GetPointer() )->GetMatrix();
    typename MatrixOffsetTransformBaseType::OffsetType inverseOffset = -( inverseMatrix * totalTransform->GetOffset() );
    for( unsigned int d = 0; d < VImageDimension; d++ )
      {
      origin[d] = inverseOffset[d];
      }
    direction = inverseMatrix;

    }
  else
    {
    totalTransform->Compose( imageTransform, true );

    typename MatrixOffsetTransformBaseType::MatrixType matrix = totalTransform->GetMatrix();
    typename MatrixOffsetTransformBaseType::OffsetType offset = totalTransform->GetOffset();

    for( unsigned int d = 0; d < VImageDimension; d++ )
      {
      origin[d] = offset[d];
      }
    direction = matrix;
    }

  image->SetDirection( direction );
  image->SetOrigin( origin );
}

template <unsigned VImageDimension>
void
RegistrationHelper<VImageDimension>
::PrintState() const
{
  this->Logger() << "Dimension = " << Self::ImageDimension << std::endl
                 << "Number of stages = " << this->m_NumberOfStages << std::endl
                 << "Use Histogram Matching " << ( this->m_UseHistogramMatching ? "true" : "false" )
                 << std::endl
                 << "Winsorize Image Intensities "
                 << ( this->m_WinsorizeImageIntensities ? "true" : "false" ) << std::endl
                 << "Lower Quantile = " << this->m_LowerQuantile << std::endl
                 << "Upper Quantile = " << this->m_UpperQuantile << std::endl;;
  for( unsigned i = 0; i < this->m_NumberOfStages; i++ )
    {
    this->Logger() << "Stage " << i + 1 << " State" << std::endl; // NOTE: + 1 for consistency.
    const Metric &          curMetric = this->m_Metrics[i];
    const TransformMethod & curTransform = this->m_TransformMethods[i];
    this->Logger() << "   Metric = " << curMetric.GetMetricAsString() << std::endl
                   << "     Fixed Image = " << curMetric.m_FixedImage << std::endl
                   << "     Moving Image = " << curMetric.m_MovingImage << std::endl
                   << "     Weighting = " << curMetric.m_Weighting << std::endl
                   << "     Sampling Strategy = "
                   << (curMetric.m_SamplingStrategy == random ? "random" : (curMetric.m_SamplingStrategy == regular )? "regular": (curMetric.m_SamplingStrategy == none )? "none" : "WARNING: UNKNOWN")
                   << std::endl
                   << "     NumberOfBins = " << curMetric.m_NumberOfBins << std::endl
                   << "     Radius = " << curMetric.m_Radius << std::endl
                   << "     Sampling percentage  = " << curMetric.m_SamplingPercentage << std::endl
                   << "   Transform = " << curTransform.XfrmMethodAsString() << std::endl
                   << "     Gradient Step = " << curTransform.m_GradientStep << std::endl
                   << "     Update Field Sigma (voxel space) = "
                   << curTransform.m_UpdateFieldVarianceInVarianceSpace << std::endl
                   << "     Total Field Sigma (voxel space) = "
                   << curTransform.m_TotalFieldVarianceInVarianceSpace << std::endl
                   << "     Update Field Time Sigma = " << curTransform.m_UpdateFieldTimeSigma << std::endl
                   << "     Total Field Time Sigma  = " << curTransform.m_TotalFieldTimeSigma << std::endl
                   << "     Number of Time Indices = " << curTransform.m_NumberOfTimeIndices << std::endl
                   << "     Number of Time Point Samples = " << curTransform.m_NumberOfTimeIndices << std::endl;
    }
}

} // namespace ants

#endif // __itkantsRegistrationHelper_hxx
