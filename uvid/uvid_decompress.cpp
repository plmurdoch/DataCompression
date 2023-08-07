/* uvid_decompress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4
   
   This placeholder code reads the (basically uncompressed) data produced by
   the uvid_compress starter code and outputs it in the uncompressed 
   YCbCr (YUV) format used for the sample video input files. To play the 
   the decompressed data stream directly, you can pipe the output of this
   program to the ffplay program, with a command like 

     ffplay -f rawvideo -pixel_format yuv420p -framerate 30 -video_size 352x288 - 2>/dev/null
   (where the resolution is explicitly given as an argument to ffplay).

   B. Bird - 2023-07-08
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include <cmath>
#include "input_stream.hpp"
#include "yuv_stream.hpp"

//Convenience function to wrap around the nasty notation for 2d vectors
template<typename T>
std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
    std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
    return V;
}

//Global vector which stores the encoding order for 8x8 Quantized DCT matrices.
std::vector<std::pair<int, int>> E_O = {
    {0,0},{0,1},{1,0},{2,0},{1,1},{0,2},{0,3},{1,2},{2,1},{3,0},{4,0},{3,1},{2,2},{1,3},{0,4},{0,5},
    {1,4},{2,3},{3,2},{4,1},{5,0},{6,0},{5,1},{4,2},{3,3},{2,4},{1,5},{0,6},{0,7},{1,6},{2,5},{3,4},
    {4,3},{5,2},{6,1},{7,0},{8,0},{7,1},{6,2},{5,3},{4,4},{3,5},{2,6},{1,7},{0,8},{0,9},{1,8},{2,7},
    {3,6},{4,5},{5,4},{6,3},{7,2},{8,1},{9,0},{10,0},{9,1},{8,2},{7,3},{6,4},{5,5},{4,6},{3,7},{2,8},
    {1,9},{0,10},{0,11},{1,10},{2,9},{3,8},{4,7},{5,6},{6,5},{7,4},{8,3},{9,2},{10,1},{11,0},{12,0},{11,1},
    {10,2},{9,3},{8,4},{7,5},{6,6},{5,7},{4,8},{3,9},{2,10},{1,11},{0,12},{0,13},{1,12},{2,11},{3,10},{4,9},
    {5,8},{6,7},{7,6},{8,5},{9,4},{10,3},{11,2},{12,1},{13,0},{14,0},{13,1},{12,2},{11,3},{10,4},{9,5},{8,6},
    {7,7},{6,8},{5,9},{4,10},{3,11},{2,12},{1,13},{0,14},{0,15},{1,14},{2,13},{3,12},{4,11},{5,10},{6,9},{7,8},
    {8,7},{9,6},{10,5},{11,4},{12,3},{13,2},{14,1},{15,0},{15,1},{14,2},{13,3},{12,4},{11,5},{10,6},{9,7},{8,8},
    {7,9},{6,10},{5,11},{4,12},{3,13},{2,14},{1,15},{2,15},{3,14},{4,13},{5,12},{6,11},{7,10},{8,9},{9,8},{10,7},
    {11,6},{12,5},{13,4},{14,3},{15,2},{15,3},{14,4},{13,5},{12,6},{11,7},{10,8},{9,9},{8,10},{7,11},{6,12},{5,13},
    {4,14},{3,15},{4,15},{5,14},{6,13},{7,12},{8,11},{9,10},{10,9},{11,8},{12,7},{13,6},{14,5},{15,4},{15,5},{14,6},
    {13,7},{12,8},{11,9},{10,10},{9,11},{8,12},{7,13},{6,14},{5,15},{6,15},{7,14},{8,13},{9,12},{10,11},{11,10},{12,9},
    {13,8},{14,7},{15,6},{15,7},{14,8},{13,9},{12,10},{11,11},{10,12},{9,13},{8,14},{7,15},{8,15},{9,14},{10,13},{11,12},
    {12,11},{13,10},{14,9},{15,8},{15,9},{14,10},{13,11},{12,12},{11,13},{10,14},{9,15},{10,15},{11,14},{12,13},{13,12},{14,11},
    {15,10},{15,11},{14,12},{13,13},{12,14},{11,15},{12,15},{13,14},{14,13},{15,12},{15,13},{14,14},{13,15},{14,15},{15,14},{15,15}
};

//Function for building the Coefficient matrix to utilize in quantization
std::vector<std::vector<double>> Coeff(){
    auto results = create_2d_vector<double>(16,16);
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            if(i == 0){
                results.at(i).at(j) = sqrt(0.0625);
            }else{
                results.at(i).at(j) = (sqrt((0.125))*cos((((2.00*j)+1.00)*i*M_PI)/32.00));
            }
        }
    }
    return results;
}

/*
Note: There were interesting interactions which resulted in me creating 2 DCT functions.
For the the High setting worked in a different way in comparison to the medium and low settings.
This was found after many hours of trial and error in the debugging process. Utilizing the method:
DCT(A) = CAC^T yielded a result in the high matrix which turned the entire image green.
However, utilizing a method DCT(A) = ACC^T kept the matrices in tact. I cannot explain how this occured.
Therefore to compensate for this we have inverse_DCT_low for medium and low values and inverse_DCT_high for the high setting values.  
*/
std::vector<std::vector<double>> inverse_DCT_low(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(x).at(j)*c.at(x).at(i));//for c transpose
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(x).at(j)); 
            }
        }
    }
    return results;
}

std::vector<std::vector<double>> inverse_DCT_high(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(i).at(x)*c.at(j).at(x));//for c transpose
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(x).at(j)); 
            }
        }
    }
    return results;
}

//Main body for the code read_input reads the input until the associated matrix of size height and width is filled completely.
//Number:length pair bytes are read and then stored in the encoded vector, once the encoded vector has enough entries for an 8x8 matrix they are offloaded into a DCT matrix.
//DCT matrix is then inverted using our coefficient in the order based on the quality. Data is then rounded and bounded to the range 0-255 for image reconstruction.
std::vector<std::vector<unsigned char>> read_input(InputBitStream input, std::vector<std::vector<unsigned char>> previous, std::vector<std::vector<int>> Q, std::vector<std::vector<double>> C, int height, int width, int quality){
    auto result = create_2d_vector<unsigned char>(height, width);
    int counter = 0;
    std::vector<unsigned char> encoded;
    int recorded_y = 0;
    int recorded_x = 0;
    int prev = input.read_byte();
    while(true){
        unsigned char num = input.read_byte();
        encoded.push_back(num);
        unsigned char length = input.read_byte();
        int value = length;
        if(value != 0){
            for(int j = 0; j<value; j++){
                encoded.push_back(num);
            }
        }
        int encoded_size = encoded.size();
        if(encoded_size == 256){
            auto DCT = create_2d_vector<int>(16,16);
            int E_O_size = E_O.size();
            for(int i = 0; i<E_O_size; i++){
                int row = E_O.at(i).first;
                int column = E_O.at(i).second;
                int for_DCT = encoded.at(i);
                DCT.at(row).at(column) = ((for_DCT-127)*Q.at(row).at(column));
            }
            encoded.clear();
            auto data = create_2d_vector<double>(16,16);
            if(quality != 2){
                data = inverse_DCT_low(DCT, C);
            }else{
                data = inverse_DCT_high(DCT, C);
            }
            for(int y = 0; y < 16; y++){
                for(int x = 0; x<16; x++){
                    if((y+recorded_y) < height){
                        if((x+recorded_x) < width){
                            int dat = 0;
                            if(prev == 1){
                                dat = round(data.at(y).at(x)+previous.at(y+recorded_y).at(x+recorded_x));
                            }
                            else{
                                dat = round(data.at(y).at(x));
                            }
                            if(dat >255){
                                dat = 255;
                            }else if(dat <0){
                                dat = 0;
                            }
                            result.at(y+recorded_y).at(x+recorded_x) = dat;
                            if (counter == ((height*width)-1)){
                                return result;
                            }else{
                                counter++;
                            }
                        }
                    }
                }
            }
            if((recorded_x+16) < width){
                recorded_x += 16;
            }else{
                recorded_x = 0;
                recorded_y += 16;
            }
            prev = input.read_byte();
        }
    }
}


int main(int argc, char** argv){

    //Note: This program must not take any command line arguments. (Anything
    //      it needs to know about the data must be encoded into the bitstream)
    
    InputBitStream input_stream {std::cin};
    unsigned int quality = input_stream.read_byte();
    //After trial and error I decided to utilize a single quantum value.
    //I utilized the resouce below to better understand standard quantum matrices.
    //I then create one that worked well with my DCT to limit the number of values which exceed the range of -127 and 127. 
    //https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossy/jpeg/coeff.htm 
    std::vector<std::vector<int>> Q = {
        {192, 128, 128, 192, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664},
        {128, 128, 192, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792},
        {128, 192, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920},
        {192, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048},
        {256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176},
        {384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304},
        {512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432},
        {640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560},
        {768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688},
        {896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816},
        {1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944},
        {1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072},
        {1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200},
        {1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328},
        {1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456},
        {1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456, 3584}
    };
    //the next for loops allow us to change the quality setting of our Quantum based on the user input.
    if(quality == 0){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = (2*Q.at(i).at(j));
            }
        }
    }else if(quality == 2){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = (round((0.2*Q.at(i).at(j)))); 
            }
        }
    }else{
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = ((Q.at(i).at(j)+8));    
            }
        }
    }
    std::vector<std::vector<double>> coeff = Coeff();
    u32 height {input_stream.read_u32()};
    u32 width {input_stream.read_u32()};
    YUVStreamWriter writer {std::cout, width, height};
    auto y_p = create_2d_vector<unsigned char>(height,width);
    auto cb_p = create_2d_vector<unsigned char>((height)/2,(width)/2);
    auto cr_p = create_2d_vector<unsigned char>((height)/2,(width)/2);
    while (input_stream.read_byte()){
        auto Y = create_2d_vector<unsigned char>(height,width);
        auto Cb = create_2d_vector<unsigned char>((height)/2,(width)/2);
        auto Cr = create_2d_vector<unsigned char>((height)/2,(width)/2);
        //Build Y matrix.
        Y = read_input(input_stream,y_p, Q, coeff,height, width, quality);
        y_p = Y;
        //Build Cb matrix.
        Cb = read_input(input_stream,cb_p, Q, coeff,((height)/2),((width)/2), quality);
        cb_p = Cb;
        //Build Cr matrix.
        Cr = read_input(input_stream, cr_p, Q, coeff,((height)/2),((width)/2), quality);
        cr_p = Cr;
        YUVFrame420& frame = writer.frame();
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                frame.Y(x,y) = Y.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cb(x,y) = Cb.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cr(x,y) = Cr.at(y).at(x);
        writer.write_frame();
    }


    return 0;
}