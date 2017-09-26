
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
    
    ASSERT(img->loadFromFile(source_path), "Unable to load image '%s'\n", source_path.c_str());
    
    sf::Vector2u image_dimensions = img->getSize();
    std::size_t image_size = image_dimensions.x * image_dimensions.y * 4;
    
    image_bytes = new sf::Uint8[image_size];
    std::memcpy(image_bytes, img->getPixelsPtr(), image_size);

    const std::string message = "A quick brown fox jumped over a lazy dog.";
    const char * message_bytes = message.c_str();

    
    std::size_t pixel_index = 0;
    // Encode message into the image
    for (std::size_t i = 0; i < message.length(); ++i)
    {
        for (std::size_t j = 0; j < 8; ++j)
        {
            std::size_t pixel_index = (i * 8 + j) * 4 + 4;
            SET_BIT(image_bytes[pixel_index], 7, GET_BIT(message[i], j));
        }
    }
    
    sf::Image result_image;
    result_image.create(image_dimensions.x, image_dimensions.y, image_bytes);
    ASSERT(result_image.saveToFile(result_path), "Unable to save iresulting image to file '%s'\n", result_path.c_str());
    
    sf::Image check_image;
    check_image.loadFromFile(result_path);
    
    char test[256] = { '\0' };

    for (std::size_t i = 0; i < message.length(); ++i)
    {
        for (std::size_t k = 0; k < 8; ++k)
        {
            std::size_t pixel_index = (i * 8 + k) * 4 + 4;
            SET_BIT(test[i], k, GET_BIT(check_image.getPixelsPtr()[pixel_index], 7));
        }
    }
    printf("%s\n", test);
    return EXIT_SUCCESS;
}
