/**
 * @file HdriToCubemap.hpp
 *
 * @brief Simple single-header library to convert an equirectangular hdri image to cubemap images. 
 *
 * @author Ingvar Out
 * Contact: ivarout2@gmail.com
 *
 * @date November 22, 2020
 */

#pragma once


#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

template <typename T>
class HdriToCubemap
{
    public:
        HdriToCubemap(const std::string& fileLocation, int cubemapResolution, bool filterLinear = true);
        ~HdriToCubemap();

        bool isHdri () const {return m_isHdri; } 
        int getCubemapResolution () const {return m_cubemapResolution;}
        int getNumChannels() const {return m_channels;}
        T** getFaces() { return m_faces; }
        T* getFront() { return m_faces[0]; }
        T* getBack(){ return m_faces[1]; }
        T* getLeft(){ return m_faces[2]; }
        T* getRight(){ return m_faces[3]; }
        T* getUp(){ return m_faces[4]; }
        T* getDown(){ return m_faces[5]; }
        void writeCubemap(const std::string& outputFolder);

    private:
        void calculateCubemap();

    private:
        bool m_isHdri;
        int m_width, m_height, m_channels;
        bool m_filterLinear;
        T* m_imageData;
        T** m_faces;
        int m_cubemapResolution;
};

template <> 
HdriToCubemap<unsigned char>::HdriToCubemap(const std::string& pathHdri, int cubemapResolution, bool filterLinear)
    : m_cubemapResolution(cubemapResolution), m_filterLinear(filterLinear)
{
    stbi_set_flip_vertically_on_load(1);
    m_isHdri = stbi_is_hdr(pathHdri.c_str());
    if (m_isHdri)
        std::cout << "Warning: image will be converted from hdr to ldr by stb_image. Use float-type template argument to create an hdr cubemap\n";
    m_isHdri = false;

    m_imageData = stbi_load(pathHdri.c_str(), &m_width, &m_height, &m_channels, 0);
    if(!m_imageData)
        throw std::runtime_error(std::string("Failed to load image ") + pathHdri);

    m_faces = new unsigned char*[6];
    for (int i = 0; i < 6; i++)
        m_faces[i] = new unsigned char[m_cubemapResolution * m_cubemapResolution * m_channels];

    calculateCubemap();
}

template <> 
HdriToCubemap<float>::HdriToCubemap(const std::string& pathHdri, int cubemapResolution, bool filterLinear)
    : m_cubemapResolution(cubemapResolution), m_filterLinear(filterLinear)
{
    m_isHdri = stbi_is_hdr(pathHdri.c_str());
    if (!m_isHdri)
        std::cout <<  "Warning: image will be converted from ldr to hdr by stb_image. Use unsigned-char-type template argument to create an ldr cubemap\n";
    m_isHdri = true;

    m_imageData = stbi_loadf(pathHdri.c_str(), &m_width, &m_height, &m_channels, 0);
    if(!m_imageData)
        throw std::runtime_error(std::string("Failed to load image ") + pathHdri);
    
    m_faces = new float*[6];
    for (int i = 0; i < 6; i++)
        m_faces[i] = new float[m_cubemapResolution * m_cubemapResolution * m_channels];

    calculateCubemap();
}

template<typename T>
HdriToCubemap<T>::~HdriToCubemap()
{
    stbi_image_free(m_imageData);
    for (int i = 0; i < 6; i++)
        delete[] *m_faces++;
}

template<typename T>
void HdriToCubemap<T>::writeCubemap(const std::string& outputFolder)
{  
    stbi_flip_vertically_on_write(1);
    std::vector<std::string> filenames = {"front", "back", "left", "right", "up", "down"};
    for (int i = 0; i < 6; i++)
    {   
        int success;
        std::string path = outputFolder + "/" + filenames[i];
        if (m_isHdri)
        {
            path += ".hdr";
            success =  stbi_write_hdr(path.c_str(), m_cubemapResolution, m_cubemapResolution, m_channels, (const float*)m_faces[i]);
        }
        else
        {
            path += ".png";
            success = stbi_write_png(path.c_str(), m_cubemapResolution, m_cubemapResolution, m_channels, (const unsigned char*)m_faces[i], 0);
        }
        if (!success)
            std::cout << "Warning: could not write '" << path << "'";
    }
}

#ifndef _USE_OPENCL // cpu implementation
template<typename T> 
void HdriToCubemap<T>::calculateCubemap()
{
    struct Vec3 {float x, y, z;};
    std::array<std::array<Vec3, 3>, 6> startRightUp= {{ // for each face, contains the 3d starting point (corresponding to left bottom pixel), right direction, and up direction in 3d space, correponding to pixel x,y coordinates of each face		{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}},  // front
        {{{1.0f, -1.0f, 1.0f},  {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}},   // back 
        {{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}},  // left
        {{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}},   // right
        {{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}},   // up
        {{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}}   // down
	}};

    for (int i = 0; i < 6; i++)
    {   
        Vec3& start = startRightUp[i][0];
        Vec3& right = startRightUp[i][1];
        Vec3& up = startRightUp[i][2];

        T* face = m_faces[i];
        Vec3 pixelDirection3d; // 3d direction corresponding to a pixel in the cubemap face
        //#pragma omp parallel for (private pixelDirection?)
        for (int row = 0; row < m_cubemapResolution; row++)
        {
            for (int col = 0; col < m_cubemapResolution; col++)
            {
                pixelDirection3d.x = start.x + ((float)col * 2.0f + 0.5f)/(float)m_cubemapResolution * right.x + ((float)row * 2.0f + 0.5f)/(float)m_cubemapResolution * up.x;
                pixelDirection3d.y = start.y + ((float)col * 2.0f + 0.5f)/(float)m_cubemapResolution * right.y + ((float)row * 2.0f + 0.5f)/(float)m_cubemapResolution * up.y;
                pixelDirection3d.z = start.z + ((float)col * 2.0f + 0.5f)/(float)m_cubemapResolution * right.z + ((float)row * 2.0f + 0.5f)/(float)m_cubemapResolution * up.z;

                float azimuth = atan2f(pixelDirection3d.x, -pixelDirection3d.z) + (float)M_PI; // add pi to move range to 0-360 deg
                float elevation = atanf(pixelDirection3d.y / sqrtf(pixelDirection3d.x * pixelDirection3d.x + pixelDirection3d.z * pixelDirection3d.z)) + (float)M_PI/2.0f;
                                
                float colHdri = (azimuth / (float)M_PI / 2.0f) * m_width; // add pi to azimuth to move range to 0-360 deg
                float rowHdri = (elevation / (float)M_PI ) * m_height;

                if (!m_filterLinear)
                {
                    int colNearest = std::clamp((int)colHdri, 0, m_width - 1);
                    int rowNearest = std::clamp((int)rowHdri, 0, m_height - 1);

                    face[col * m_channels + m_cubemapResolution * row * m_channels] = m_imageData[colNearest * m_channels + m_width * rowNearest * m_channels] ; // red
                    face[col * m_channels + m_cubemapResolution * row * m_channels + 1] = m_imageData[colNearest * m_channels + m_width * rowNearest * m_channels + 1]; //green
                    face[col * m_channels + m_cubemapResolution * row * m_channels + 2] = m_imageData[colNearest * m_channels + m_width * rowNearest * m_channels + 2]; //blue
                    if(m_channels > 3)
                        face[col * m_channels + m_cubemapResolution * row * m_channels + 3] = m_imageData[colNearest*m_channels + m_width * rowNearest * m_channels + 3]; //alpha
                }
                else // perform bilinear interpolation
                {
                    float intCol, intRow;
                    float factorCol = modf(colHdri- 0.5f, &intCol);        // factor gives the contribution of the next column, while the contribution of intCol is 1 - factor
                    float factorRow = modf(rowHdri - 0.5f, &intRow);

                    int low_idx_row = static_cast<int>(intRow);
                    int low_idx_column = static_cast<int>(intCol);
                    int high_idx_column;
                    if (factorCol < 0.0f)                           //modf can only give a negative value if the azimuth falls in the first pixel, left of the center, so we have to mix with the pixel on the opposite side of the panoramic image
                        high_idx_column = m_width - 1;
                    else if (low_idx_column == m_width -1)          //if we are in the right-most pixel, and fall right of the center, mix with the left-most pixel
                        high_idx_column = 0;
                    else 
                        high_idx_column = low_idx_column + 1;

                    int high_idx_row;
                    if (factorRow < 0.0f)                           
                        high_idx_row = m_height - 1;
                    else if (low_idx_row == m_height - 1)
                        high_idx_row = 0;
                    else 
                        high_idx_row = low_idx_row + 1;

                    factorCol = abs(factorCol);
                    factorRow = abs(factorRow);
                    float f1 = (1 - factorRow) * (1 - factorCol);
                    float f2 = factorRow * (1 - factorCol);
                    float f3 = (1 - factorRow) * factorCol;
                    float f4 = factorRow * factorCol;

                    for (int j = 0; j < m_channels; j++)
                    {   
                        unsigned char interpolatedValue = static_cast<unsigned char>(m_imageData[low_idx_column * m_channels + m_width * low_idx_row * m_channels + j] * f1 + 
                                                    m_imageData[low_idx_column * m_channels + m_width * high_idx_row * m_channels + j] * f2 + 
                                                    m_imageData[high_idx_column * m_channels + m_width * low_idx_row * m_channels + j] * f3 + 
                                                    m_imageData[high_idx_column * m_channels + m_width * high_idx_row * m_channels + j] * f4);
                        face[col * m_channels + m_cubemapResolution * row * m_channels + j] = std::clamp(interpolatedValue, (uint8_t)0, (uint8_t)255);
                    }
                }
            }
        }
    }
}
#else // opencl implementation

#include <CL/cl.hpp>

template<typename T> 
void HdriToCubemap<T>::calculateCubemap()
{
    struct Vec3 {float x, y, z;};
    std::array<std::array<Vec3, 3>, 6> startRightUp= {{ // for each face, contains the 3d starting point (corresponding to left bottom pixel), right direction, and up direction in 3d space, correponding to pixel x,y coordinates of each face		{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}},  // front
        {{{1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}},   // back 
        {{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}},  // left
        {{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}},   // right
        {{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}},   // up
        {{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}}   // down
	}};
    
    // opencl seems to require RGBA format (requiring 4 channels on 3 channel images for large hdri's causes stb_image to fail)
    if (m_channels == 3)
    {
        T* data = new T[m_width * m_height * 4];
        int idx = 0;
        for (int i = 0; i < m_width * m_height; i++)
        {
            data[idx++] = m_imageData[i * 3];
            data[idx++] = m_imageData[i * 3 + 1];
            data[idx++] = m_imageData[i * 3 + 2];
            data[idx++] = (T)255; //TODO: depends on whether type is float or unsigned char!!
        }
        delete[] m_imageData;
        m_imageData = data;
        for (int i = 0; i < 6; i++)
        {
            delete[] m_faces[i];
            m_faces[i] = new T[m_cubemapResolution * m_cubemapResolution * 4];
        }
        m_channels = 4;
    }

    // create cl program
    std::string pathToClFile = std::string(HDRITOCUBEMAP_CL_DIR) + "/ProcessFace.cl";
    std::ifstream clFile(pathToClFile.c_str());
    std::string src(std::istreambuf_iterator<char>(clFile), (std::istreambuf_iterator<char>()));
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    auto platform = platforms.front();
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    auto device = devices.front();
    auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
    
    cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
    cl::Context context(device);
    cl::Program program(context, sources);
    cl_int err;
    if (m_filterLinear)
        err = program.build("-cl-std=CL1.2 -D filter=CLK_FILTER_LINEAR");
    else
        err = program.build("-cl-std=CL1.2 -D filter=CLK_FILTER_NEAREST");
    if (err != 0)
        throw std::runtime_error(std::string("cl error code: ") + std::to_string(err));
        
    // create and write image
    cl::ImageFormat format;
    if (m_isHdri)
        format = cl::ImageFormat(CL_RGBA, CL_FLOAT);  //TODO: make formate depend on nmber of channels
    else
        format = cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8); 

    // cl::Image2D imgHdri(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, format, m_width, m_height);
    cl::Image2D imgHdri(context, CL_MEM_READ_ONLY, format, m_width, m_height);
    cl::Image2D imgFace(context, CL_MEM_WRITE_ONLY, format, m_cubemapResolution, m_cubemapResolution);
    cl::CommandQueue queue(context, device);
    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = m_width;
    region[1] = m_height;
    region[2] = 1;
    queue.enqueueWriteImage(imgHdri, CL_TRUE, origin, region, 0, 0, m_imageData);
    cl::Buffer bufDirections(context, CL_MEM_READ_ONLY, sizeof(float) * 9);
    
    for (int i = 0; i < 6; i++)
    {   
        T* face = m_faces[i];

        cl::Kernel kernel(program, "processFace");
        kernel.setArg(0, imgHdri);
        kernel.setArg(1, imgFace);
        kernel.setArg(2, bufDirections);
        
        err = queue.enqueueWriteBuffer(bufDirections, CL_TRUE, 0, 9 * sizeof(float), &startRightUp[i][0]);
        err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(m_cubemapResolution, m_cubemapResolution));
        cl::finish();
    
        cl::size_t<3> origin_out;
        origin_out[0] = 0;
        origin_out[1] = 0;
        origin_out[2] = 0; 
        cl::size_t<3> region_out;
        region_out[0] = m_cubemapResolution;
        region_out[1] = m_cubemapResolution;
        region_out[2] = 1;

        queue.enqueueReadImage(imgFace, CL_TRUE, origin_out, region_out, 0, 0, face);
        cl::finish(); 
    }
}
#endif