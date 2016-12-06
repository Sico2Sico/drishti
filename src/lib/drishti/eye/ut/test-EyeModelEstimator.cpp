/*!
  @file   test-EyeModelEstimator.cpp
  @author David Hirvonen
  @brief  Google fixture with various subtests for the drishti public SDK.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest.h>

// These must come before drishti_cv.hpp
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#if DRISHTI_SERIALIZE_WITH_BOOST
#  include "drishti/core/boost_serialize_common.h"
#endif

// DRISHTI_SERIALIZE_EYE_WITH_CEREAL: provides local json serialization of the eye type
// and this setting may be used independently from DRISHTI_SERIALIZE_WITH_CEREAL
#define DRISHTI_SERIALIZE_EYE_WITH_CEREAL 1

#if DRISHTI_SERIALIZE_WITH_CEREAL || DRISHTI_SERIALIZE_EYE_WITH_CEREAL
#  include "drishti/core/drishti_stdlib_string.h"
#  include "drishti/core/drishti_cereal_pba.h"
#  include "drishti/core/drishti_cv_cereal.h"
#endif

#if DRISHTI_SERIALIZE_EYE_WITH_CEREAL
#  include <cereal/archives/json.hpp>
#  include <cereal/archives/xml.hpp>
#endif

#include "drishti/eye/EyeModelEstimator.h"
#include "drishti/rcpr/CPR.h"

#include <fstream>

// https://code.google.com/p/googletest/wiki/Primer

const char* modelFilename;
const char* imageFilename;
const char* truthFilename;
const char* outputDirectory;
bool isTextArchive;

#define BEGIN_EMPTY_NAMESPACE namespace {
#define END_EMPTY_NAMESPACE }

#define EYE_ASPECT_RATIO (4.0/3.0)

BEGIN_EMPTY_NAMESPACE

static cv::Point padToAspectRatio(const cv::Mat &image, cv::Mat &padded, double aspectRatio);
static float detectionScore(const drishti::eye::EyeModel &eyeA, const drishti::eye::EyeModel &eyeB, const cv::Size &size, float scale);

class EyeModelEstimatorTest : public ::testing::Test
{
public:
    
    static std::shared_ptr<drishti::eye::EyeModelEstimator> create(const std::string &filename, bool textArchive)
    {
        auto estimator = std::make_shared<drishti::eye::EyeModelEstimator>();
#if DRISHTI_SERIALIZE_WITH_BOOST        
#if DRISHTI_USE_TEXT_ARCHIVES
        if(textArchive)
        {
            drishti::eye::EyeModelEstimator::loadTXT(modelFilename, *estimator);
        }
        else
#endif
        {
            drishti::eye::EyeModelEstimator::loadPBA(modelFilename, *estimator);
        }
#endif
        return estimator;
    }
    
protected:

    struct Entry
    {
        cv::Mat image;
        bool isRight;
    };
 
    // Setup
    EyeModelEstimatorTest()
    {
        // Create the segmenter (constructor tests performed prior to this)
        m_eyeSegmenter = create(modelFilename, isTextArchive);

        // Load the ground truth data:
        loadTruth();

        // Load sample for each image width:
        loadImages();
    }

    // Cleanup
    virtual ~EyeModelEstimatorTest()
    {

    }

    // Called after constructor for each test
    virtual void SetUp() {}

    // Called after destructor for each test
    virtual void TearDown() {}

    // Utility methods:
    void loadImages()
    {
        assert(imageFilename);

        // First get our 4x3 aspect ratio image
        cv::Mat image = cv::imread(imageFilename, cv::IMREAD_COLOR);
        assert(!image.empty());
        
        m_image = image;

        cv::Mat padded;
        cv::Rect roi({0,0}, image.size());
        padToAspectRatio(image, padded, 4.0/3.0);
        assert(!padded.empty());

        // Next create each possible size from 0 to 1000:
        for(int width = 0; width < 512; width++)
        {
            cv::Mat resized;
            if(width > 0)
            {
                int height(float(width) / EYE_ASPECT_RATIO + 0.5f);
                cv::resize(padded, resized, {width, height}, width);
            }

            {
                // Create right image
                Entry entry { resized, true };
                m_images.emplace_back(entry);
            }
        }
    }

    void loadTruth()
    {
        assert(truthFilename);
        std::ifstream is(truthFilename);
        if(is.good())
        {
#if DRISHTI_SERIALIZE_EYE_WITH_CEREAL
            auto eye = std::make_shared<drishti::eye::EyeModel>();
            cereal::JSONInputArchive ia(is);
            typedef decltype(ia) Archive;
            ia( GENERIC_NVP("eye", *eye) );
            m_eye = eye;
            m_eye->refine();            
#else
            std::cerr << "Skipping JSON archive" << std::endl;
#endif

        }
        else
        {
            std::cerr << "Unable to find ground truth file: " << truthFilename << std::endl;
            std::cerr << "Make sure test-drishti-drishti is run first." << std::endl;
        }
    }
    
    void createImage(Entry &entry, int rows, int cols, const cv::Vec3b &color)
    {
        entry.image.create(rows, cols, CV_8UC3);
        entry.isRight = true;
    }

    // Objects declared here can be used by all tests in the test case for EyeModelEstimator.
    std::shared_ptr<drishti::eye::EyeModelEstimator> m_eyeSegmenter;

    // Test images:
    std::vector<Entry> m_images;
    
    int m_targetWidth = 127;
    
    // Ground truth image:
    cv::Mat m_image;

    // Ground truth:
    std::shared_ptr<drishti::eye::EyeModel> m_eye;

    // Score tolerance:
    float m_scoreThreshold = 0.60;
};

/*
 * Basic class construction
 */

TEST(EyeModelEstimator, StringConstructor)
{
    // Make sure modelFilename is not null:
    ASSERT_NE(modelFilename, (const char *)NULL);

    auto segmenter = EyeModelEstimatorTest::create(modelFilename, isTextArchive);
    
    // Make sure we reached this point (no exceptions):
    ASSERT_EQ(segmenter->good(), true);
}

#if DRISHTI_SERIALIZE_WITH_CEREAL
TEST_F(EyeModelEstimatorTest, CerealSerialization)
{
    // Make sure modelFilename is not null:
    ASSERT_NE(modelFilename, (const char *)NULL);
    
    std::string filename = std::string(outputDirectory) + "/eye.cpb";
    {
        std::ofstream ofs(filename, std::ios::binary);
        if(ofs)
        {
            cereal::PortableBinaryOutputArchive3 oa(ofs);
            oa << *m_eyeSegmenter;
        }
    }
    
    drishti::eye::EyeModelEstimator segmenter2;
    {
        std::ifstream ifs(filename, std::ios::binary);
        if(ifs)
        {
            cereal::PortableBinaryInputArchive3 ia(ifs);
            ia >> segmenter2;
        }
    }
    
//    int i = 128;
//    
//    assert(m_images[i].isRight);
//    drishti::eye::EyeModel eye;
//    int code = segmenter2(m_images[i].image, eye);
//    EXPECT_EQ(code, 0);
//    
//    cv::Mat mask = cv::Mat(m_images[i].image.size(), CV_8UC3, cv::Scalar::all(0));
//    eye.draw(mask);
//    cv::imshow("mask", mask); cv::waitKey(0);
}
#endif // DRISHTI_SERIALIZE_WITH_CEREAL


TEST(EyeModelEstimator, StreamConstructor)
{
    // Make sure modelFilename is not null:
    ASSERT_NE(modelFilename, (const char *)NULL);

    std::ifstream is(modelFilename);

    // Make sure file could be opened:
    ASSERT_TRUE((bool)is);

    auto segmenter = EyeModelEstimatorTest::create(modelFilename, isTextArchive);

    // Make sure we reached this point (no exceptions):
    EXPECT_EQ(segmenter->good(), true);
}

static void checkValid(const drishti::eye::EyeModel &eye, const cv::Size &size)
{
    EXPECT_GT(eye.roi->width, 0);
    EXPECT_GT(eye.roi->height, 0);
    EXPECT_GE(eye.eyelids.size(), 16);
    EXPECT_GT(eye.irisEllipse.size.width, 0);
    EXPECT_GT(eye.pupilEllipse.size.width, 0);
}

/*
 * Fixture tests
 */

TEST_F(EyeModelEstimatorTest, EyeSerialization)
{
    drishti::eye::EyeModel eye;
    
    assert(m_images[m_targetWidth].isRight);
    /* int code = */ (*m_eyeSegmenter)(m_images[m_targetWidth].image, eye);

#if DRISHTI_SERIALIZE_EYE_WITH_CEREAL
    std::string filename(outputDirectory);
    filename += "/right_eye_2.json";

    std::ofstream os(filename);
    cereal::JSONOutputArchive oa(os);
    typedef decltype(oa) Archive;
    oa << GENERIC_NVP("eye", eye);
#else
    std::cerr << "Skipping JSON archive" << std::endl;
#endif
}

// TODO: Add ground truth comparison
// * hamming distance for sclera and iris masks components
TEST_F(EyeModelEstimatorTest, ImageValid)
{
    if(!m_eye)
    {
        return;
    }
    for(int i = 32; i < m_images.size(); i++)
    {
        // Make sure image has the expected size:
        EXPECT_EQ(m_images[i].image.cols, i);

        assert(m_images[i].isRight);
        drishti::eye::EyeModel eye;
        int code = (*m_eyeSegmenter)(m_images[i].image, eye);
        EXPECT_EQ(code, 0);
        
        //cv::Mat mask = cv::Mat(m_images[i].image.size(), CV_8UC3, cv::Scalar::all(0));
        //eye.draw(mask);
        //cv::imshow("mask", mask); cv::waitKey(0);
        
        eye.refine();
        checkValid(eye, m_images[i].image.size());

        // Ground truth comparison for reasonable resolutions
        if(i > 64)
        {
            const float threshold = (i == m_targetWidth) ? m_scoreThreshold : 0.5;
            const float scaleGroundTruthToCurrent = float(m_images[i].image.cols) / float(m_targetWidth);
            const float score = detectionScore(*m_eye, eye, m_images[i].image.size(), scaleGroundTruthToCurrent);
            ASSERT_GT(score, threshold);
        }
    }
}

// Currently there is no internal quality check, but this is included for regression:
TEST_F(EyeModelEstimatorTest, ImageIsBlack)
{
    Entry entry;
    int width = 256;
    int height = int(float(width) / EYE_ASPECT_RATIO + 0.5f);
    createImage(entry, height, width, {0,0,0});

    assert(entry.isRight);
    drishti::eye::EyeModel eye;
    int code = (*m_eyeSegmenter)(entry.image, eye);
    EXPECT_EQ(code, 0);
    checkValid(eye, entry.image.size());
}

TEST_F(EyeModelEstimatorTest, ImageIsWhite)
{
    Entry entry;
    int width = 256;
    int height = int(float(width) / EYE_ASPECT_RATIO + 0.5f);
    createImage(entry, height, width, {0,0,0});

    assert(entry.isRight);
    drishti::eye::EyeModel eye;
    int code = (*m_eyeSegmenter)(entry.image, eye);
    EXPECT_EQ(code, 0);
    checkValid(eye, entry.image.size());
}

// #######

static cv::Mat scleraMask(const drishti::eye::EyeModel &eye, const cv::Size &size)
{
    return eye.mask(size);
}

static cv::Point padToAspectRatio(const cv::Mat &image, cv::Mat &padded, double aspectRatio)
{
    CV_Assert(image.channels() == 3);

    int top = 0, left = 0, bottom = 0, right = 0;
    if(double(image.cols)/image.rows > aspectRatio)
    {
        int padding = int(double(image.cols) / aspectRatio + 0.5) - image.rows;
        top = padding / 2;
        bottom = padding - top;
    }
    else
    {
        int padding = int(double(image.rows) * aspectRatio + 0.5) - image.cols;
        left = padding / 2;
        right = padding - left;
    }

    cv::Scalar mu = cv::mean(image);
    cv::copyMakeBorder(image, padded, top, bottom, left, right, cv::BORDER_CONSTANT, mu);

    return cv::Point(left, top);
}

// eyeA: ground truth eye
// eyeB: test eye
static float
detectionScore(const drishti::eye::EyeModel &eyeA, const drishti::eye::EyeModel &eyeB, const cv::Size &size, float scale)
{
    drishti::eye::EyeModel eyeGT = eyeA * scale;
    cv::Mat maskGT = scleraMask(eyeGT, size);
    cv::Mat maskB = scleraMask(eyeB, size);

    int numerator = 0, denominator = 0;
    try
    {
        numerator = cv::countNonZero(maskGT & maskB);
    }
    catch(...) {}
    try
    {
        denominator = cv::countNonZero(maskGT | maskB);
    }
    catch(...) {}
    float score = denominator ? float(numerator) / (denominator) : 0;

#define DEBUG_PASCAL 0
#if DEBUG_PASCAL
    {
        std::cout << "SCORE: " << score << std::endl;
        cv::imshow("maskA", maskGT); // opt
        cv::imshow("maskB", maskB); // opt
        int i = 0;
        cv::Mat tmp[2]  = { maskGT, maskB };
        do { cv::imshow("a", tmp[i++%2]); } while(cv::waitKey(0) != int('q'));
        cv::waitKey(0);
    }
#endif

    return score;
}

END_EMPTY_NAMESPACE


