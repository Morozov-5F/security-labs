
//
// Disclaimer:
// ----------
//
// This code will work only if you selected window, graphics and audio.
//
// In order to load the resources like cute_image.png, you have to set up
// your target scheme:
//
// - Select "Edit Scheme…" in the "Product" menu;
// - Check the box "use custom working directory";
// - Fill the text field with the folder path containing your resources;
//        (e.g. your project folder)
// - Click OK.
//

#include <SFML/Graphics.hpp>


#include <iostream>
#include <string>

#include <cstdio>
#include <cstring>

#include <unistd.h>

#include <cstdlib>

#include "zlib.h"

extern "C" {
#include "FeistelNet.h"
}

#define ASSERT(cond, fmt, ...)                          \
do                                                      \
{                                                       \
    if (!cond)                                          \
    {                                                   \
        std::printf(fmt, __VA_ARGS__);                  \
        exit(1);                                        \
    }                                                   \
} while (0);

#define GET_BIT(byte, index)        (((byte) >> (index)) & 1)
#define SET_BIT(byte, index, value) (byte ^= (-(value) ^ (byte)) & (1 << (index)));

int main(int argc, char const** argv)
{
    std::string source_path = "source_image.png";
    std::string result_path = "result_image.png";
    
    sf::Image * img = new sf::Image;
    sf::Uint8 * image_bytes = nullptr;
    
    ASSERT(img->loadFromFile(source_path), "Unable to load image '%s'\n",
           source_path.c_str());
    
    sf::Vector2u image_dimensions = img->getSize();
    std::size_t image_size = image_dimensions.x * image_dimensions.y * 4;
    
    image_bytes = new sf::Uint8[image_size];
    std::memcpy(image_bytes, img->getPixelsPtr(), image_size);

    sf::Image black_box;
    black_box.create(800, 500, sf::Color::Black);
    
    sf::Texture areaToLoad;
    areaToLoad.loadFromImage(*img, sf::IntRect(0, 0, 800, 500));
    sf::Uint8 * pixels_to_encode = new sf::Uint8[800 * 500 * 4];
    sf::Uint8 * encoded_pixels = new sf::Uint8[800 * 500 * 4];
    std::size_t encoded_size = 0;
    sf::Image areaImage = areaToLoad.copyToImage();
    std::memcpy(pixels_to_encode, areaImage.getPixelsPtr(),
                800 * 500 * 4);
    
    z_stream deflate_stream;
    deflate_stream.zalloc = Z_NULL;
    deflate_stream.zfree  = Z_NULL;
    deflate_stream.opaque = Z_NULL;
    deflate_stream.avail_in = deflate_stream.avail_out = 800 * 500 * 4;
    deflate_stream.next_in  = pixels_to_encode;
    deflate_stream.next_out = encoded_pixels;
    
    deflateInit(&deflate_stream, Z_BEST_COMPRESSION);
    deflate(&deflate_stream, Z_FINISH);
    deflateEnd(&deflate_stream);
    
    std::cout << "Full size:       " << image_size << std::endl;
    std::cout << "Previous size:   " << 800 * 500 * 4 << std::endl;
    std::cout << "Compressed size: " << std::strlen((const char *)encoded_pixels) << std::endl;

    feist_ctx_t * ctx = feist_init(10, 0xDEAD, NULL, NULL);
    
    std::size_t deflated_size = std::strlen((const char *)encoded_pixels);
    char * encrypted = feist_encrypt(ctx, (const char *)encoded_pixels,
                                     &deflated_size);
    std::size_t pixel_index = 0;
    
    // Encode message into the image
    for (std::size_t i = 0; i < deflated_size; ++i)
    {
        for (std::size_t j = 0; j < 8; ++j)
        {
            std::size_t pixel_index = (i * 8 + j) * 4 + 4;
            while (sf::IntRect(0, 0, 800, 500).contains(pixel_index / image_dimensions.x,
                                                        pixel_index % image_dimensions.x))
            {
                pixel_index++;
                std::cout << pixel_index / image_dimensions.x << " " << pixel_index % image_dimensions.x << std::endl;
            }
            SET_BIT(image_bytes[pixel_index], 7, GET_BIT(encrypted[i], j));
        }
    }
    sf::Texture result_texture;
    result_texture.create(image_dimensions.x, image_dimensions.y);
    result_texture.update(image_bytes);
//    result_texture.update(black_box, 0, 0);
    auto result_image = result_texture.copyToImage();
    ASSERT(result_image.saveToFile(result_path), "Unable to save resulting image to file '%s'\n", result_path.c_str());
    
    return EXIT_SUCCESS;
}
