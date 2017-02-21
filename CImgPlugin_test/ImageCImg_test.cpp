/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2017 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU General Public License as published by the Free  *
* Software Foundation; either version 2 of the License, or (at your option)   *
* any later version.                                                          *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
* more details.                                                               *
*                                                                             *
* You should have received a copy of the GNU General Public License along     *
* with this program. If not, see <http://www.gnu.org/licenses/>.              *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#include <CImgPlugin/ImageCImg.h>

#include <sofa/helper/system/FileRepository.h>
#include <cstring>
#include <algorithm>
#include <numeric>

#include <gtest/gtest.h>

#include <SofaTest/TestMessageHandler.h>
using sofa::helper::logging::MessageDispatcher ;
using sofa::helper::logging::ExpectMessage ;
using sofa::helper::logging::Message ;

#include <sofa/helper/logging/LoggingMessageHandler.h>
using sofa::helper::logging::MainLoggingMessageHandler ;

#include <sofa/helper/logging/CountingMessageHandler.h>
using sofa::helper::logging::MainCountingMessageHandler ;


namespace sofa {


class ImageCImg_test : public ::testing::Test
{
protected:
    ImageCImg_test() {
        MessageDispatcher::clearHandlers() ;
        MessageDispatcher::addHandler( &MainCountingMessageHandler::getInstance() ) ;
        MessageDispatcher::addHandler( &MainLoggingMessageHandler::getInstance() ) ;
    }

    void SetUp()
    {
        sofa::helper::system::DataRepository.addFirstPath(CIMGPLUGIN_RESOURCES_DIR);
    }
    void TearDown()
    {
        sofa::helper::system::DataRepository.removePath(CIMGPLUGIN_RESOURCES_DIR);
    }

    bool checkExtension(const std::string& ext)
    {
        std::vector<std::string>::const_iterator extItBegin = sofa::helper::io::ImageCImgCreators::cimgSupportedExtensions.cbegin();
        std::vector<std::string>::const_iterator extItEnd = sofa::helper::io::ImageCImgCreators::cimgSupportedExtensions.cend();

        return (std::find(extItBegin, extItEnd, ext) != extItEnd);
    }

    struct ImageCImgTestData
    {
        //used to compare lossy images
        static constexpr float PIXEL_TOLERANCE = 1; //1 pixel of difference on the average of the image

        std::string filename;
        unsigned int width;
        unsigned int height;
        unsigned int bpp;
        const unsigned char* data;

        ImageCImgTestData(const std::string& filename, unsigned int width, unsigned int height
            , unsigned int bpp, const unsigned char* data)
            : filename(filename), width(width), height(height), bpp(bpp), data(data)
        {

        }

        bool comparePixels(bool lossy, const unsigned char* testdata)
        {
            bool res = true;
            if(lossy)
            {
                unsigned int total = width*height*bpp;
                //we will compare the average of pixels
                //and it has to be within a certain ratio with the reference
                //there are much better algorithms
                //but that is not the really the point here.

                float totalRef = std::accumulate(data, data + total, 0, std::plus<unsigned int>());
                float totalTest = std::accumulate(testdata, testdata + total, 0, std::plus<unsigned int>());

                res = abs(totalRef - totalTest)/total < PIXEL_TOLERANCE;

            }
            else
            {
                res = (0 == std::memcmp(data, testdata, width*height*bpp));
            }

            return res;
        }

        void testBench(bool lossy = false)
        {
            sofa::helper::io::ImageCImg img;

            bool isLoaded = img.load(filename);
            ASSERT_TRUE(isLoaded);
            //necessary to test if the image was effectively loaded
            //otherwise segfault from Image (and useless to test the rest anyway)

            ASSERT_EQ(width, img.getWidth());
            ASSERT_NE(width + 123 , img.getWidth());
            ASSERT_EQ(height, img.getHeight());
            ASSERT_NE(height + 41, img.getHeight());
            ASSERT_EQ(width*height, img.getPixelCount());
            ASSERT_NE(width*height+11, img.getPixelCount());

            ASSERT_EQ(bpp, img.getBytesPerPixel());
            ASSERT_NE(bpp-2, img.getBytesPerPixel());

            const unsigned char* testdata = img.getPixels();
            ASSERT_TRUE(comparePixels(lossy, testdata));
            //add errors
            unsigned char* testdata2 = img.getPixels();
            std::for_each(testdata2, testdata2+width*height*bpp, [](unsigned char &n){ n+=PIXEL_TOLERANCE + 1; });

            ASSERT_FALSE(comparePixels(lossy, testdata2));
        }
    };

};

TEST_F(ImageCImg_test, ImageCImg_NoFile)
{
    /// This generate a test failure if no error message is generated.
    ExpectMessage raii(Message::Error) ;

    sofa::helper::io::ImageCImg imgNoFile;
    EXPECT_FALSE(imgNoFile.load("randomnamewhichdoesnotexist.png"));
}

TEST_F(ImageCImg_test, ImageCImg_NoImg)
{
    sofa::helper::io::ImageCImg imgNoImage;
    EXPECT_FALSE(imgNoImage.load("imagetest_noimage.png"));
}

TEST_F(ImageCImg_test, ImageCImg_ReadBlackWhite)
{
    unsigned int width = 800;
    unsigned int height = 600;
    unsigned int bpp = 3;
    unsigned int totalsize = width*height*bpp;

    unsigned char* imgdata = new unsigned char[totalsize];
    //half image (800x300) is black the other one is white
    std::fill(imgdata, imgdata + (800*300*3), 0);
    std::fill(imgdata + (800 * 300 * 3), imgdata + (800 * 600 * 3), 255);


    if(checkExtension("png"))
    {
        ImageCImgTestData imgBW("imagetest_blackwhite.png", width, height, bpp, imgdata);
        imgBW.testBench();
    }
    if(checkExtension("jpg"))
    {
        ImageCImgTestData imgBW("imagetest_blackwhite.jpg", width, height, bpp, imgdata);
        imgBW.testBench(true);
    }
    if(checkExtension("tiff"))
    {
        ImageCImgTestData imgBW("imagetest_blackwhite.tiff", width, height, bpp, imgdata);
        imgBW.testBench();
    }
    if(checkExtension("bmp"))
    {
        ImageCImgTestData imgBW("imagetest_blackwhite.bmp", width, height, bpp, imgdata);
        imgBW.testBench();
    }
}


}// namespace sofa