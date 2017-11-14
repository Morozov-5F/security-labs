#include <iostream>
#include <csetjmp>
#include <cassert>
#include <fstream>

#include <map>
#include <vector>

extern "C"
{
#include <jpeglib.h>
#include <getopt.h>
}

typedef struct error_mgr_s
{
    struct jpeg_error_mgr errm_jpeg_mgr;
    jmp_buf               errm_setjmp_buf;
} error_mgr_t;

static void
s_error_mgr_exit(j_common_ptr cinfo)
{
    error_mgr_t * error = (error_mgr_t *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);

    longjmp(error->errm_setjmp_buf, 1);
}

static JBLOCKARRAY *
s_read_coefficients(jpeg_decompress_struct * decompress_info, jvirt_barray_ptr * coefficients)
{
    JBLOCKARRAY * row_pointers = nullptr;

    assert(nullptr != coefficients);
    assert(nullptr != decompress_info);

    row_pointers = new JBLOCKARRAY[3];
    assert(nullptr != row_pointers);

    // Iterate through component matrices (Y, Cb, Cr)
    for (JDIMENSION component_num = 0; component_num < decompress_info->num_components; ++component_num)
    {
        jpeg_component_info * component_info = &decompress_info->comp_info[component_num];
        std::cout << "Reading " << component_num << " component. . ." << std::endl;

        size_t height_in_blocks = component_info->height_in_blocks;
        size_t width_in_blocks  = component_info->width_in_blocks;

        std::cout << "Component parameters: " << std::endl;
        std::cout << "          width in blocks: " << component_info->width_in_blocks  << std::endl;
        std::cout << "         height in blocks: " << component_info->height_in_blocks << std::endl;
        std::cout << "              total width: " << component_info->width_in_blocks  * DCTSIZE2 << std::endl;
        std::cout << "             total height: " << component_info->height_in_blocks * DCTSIZE2 << std::endl;
        std::cout << "            v_samp_factor: " << component_info->v_samp_factor << std::endl;

        // Assign component pointer
        row_pointers[component_num] = decompress_info->mem->access_virt_barray((j_common_ptr) &decompress_info,
                                                                              coefficients[component_num],
                                                                              0,
                                                                              component_info->v_samp_factor,
                                                                              FALSE);
        for (JDIMENSION row_num = 0; row_num < height_in_blocks; ++row_num)
        {
            for (JDIMENSION block_num = 0; block_num < width_in_blocks; ++block_num)
            {
                for (JDIMENSION i = 0; i < DCTSIZE2; ++i)
                {
                    short dc = row_pointers[component_num][row_num][block_num][i];
                }
            }
        }
    }
    return row_pointers;
}

static void
s_calculate_distribution_for_rows(jpeg_component_info * component_info, JBLOCKARRAY component)
{
    std::map<short, unsigned> coefficient_map_row;

    assert(nullptr != component);
    assert(nullptr != component_info);

    std::cout << "Calculating coefficients distribution for each row... ";
    size_t total_coeffs = 0;
    std::vector<double> distributions_row;

    for (auto i = 0; i < component_info->height_in_blocks; ++i)
    {
        for (auto j = 0; j < component_info->width_in_blocks; ++j)
        {
            for (auto k = 1; k < DCTSIZE2; ++k)
            {
                if (component[i][j][k] == 1 || component[i][j][k] == 0)
                {
                    continue;
                }
                auto coeff = component[i][j][k] & 0b00000011;
                coefficient_map_row[coeff] += 1;
                ++total_coeffs;
            }
        }
        std::cout << "Row number " << i << std::endl;
        for (auto iter = coefficient_map_row.begin(); iter != coefficient_map_row.end(); ++iter)
        {
            std::cout << iter->first << " " << iter->second << std::endl;
        }
        coefficient_map_row.clear();
        total_coeffs = 0;
    }
}

static void
s_make_coeff_histogram(jpeg_component_info * component_info, JBLOCKARRAY component)
{
    std::map<short, unsigned> coefficient_map;

    assert(nullptr != component);
    assert(nullptr != component_info);

    std::cout << "Creating JPEG coefficients histogram... ";
    size_t total_coeffs = 0;

    for (auto i = 0; i < component_info->height_in_blocks; ++i)
    {
        for (auto j = 0; j < component_info->width_in_blocks; ++j)
        {
            for (auto k = 1; k < DCTSIZE2; ++k)
            {
                if (component[i][j][k] == 1 || component[i][j][k] == 0)
                {
                    continue;
                }
                auto coeff = component[i][j][k] & 0b00000011;
                coefficient_map[coeff] += 1;
                ++total_coeffs;
            }
        }
    }
    std::cout << "Done!" << std::endl;

    std::ofstream out_str("histogram_result.out");
    out_str << "# JPEG coefficients histogram" << std::endl;
    out_str << "# coefficient, frequency" << std::endl;
    for (auto iter = coefficient_map.begin(); iter != coefficient_map.end(); ++iter)
    {
        out_str << iter->first << " " << iter->second << std::endl;
    }
    out_str.close();
    return;
}

static int
check_jpeg_coeffs_hist()
{

}

static int
check_jpeg_coeffs_lsb_transitions()
{

}

int
process_jpeg_file(std::string filename)
{
    JBLOCKARRAY * row_pointers = nullptr;
    struct jpeg_decompress_struct cinfo;
    error_mgr_t                   jerr;

    jvirt_barray_ptr  * jpeg_coefficients = nullptr;

    FILE * in_file = nullptr;

    in_file = fopen(filename.c_str(), "rb");
    if (nullptr == in_file)
    {
        std::cout << "Unable to open file " << filename << std::endl;
        return EXIT_FAILURE;
    }

    cinfo.err = jpeg_std_error(&jerr.errm_jpeg_mgr);
    jerr.errm_jpeg_mgr.error_exit = s_error_mgr_exit;

    int jump_ret = setjmp(jerr.errm_setjmp_buf);
    if (0 != jump_ret)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(in_file);
        std::cout << "Unable to open a JPEG file!" << std::endl;
        return EXIT_FAILURE;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, in_file);

    if (JPEG_HEADER_OK != jpeg_read_header(&cinfo, FALSE))
    {
        std::cout << "Unable to read image header!" << std::endl;
    }

    jpeg_coefficients = jpeg_read_coefficients(&cinfo);
    row_pointers = s_read_coefficients(&cinfo, jpeg_coefficients);

    auto component_index = 0;
    s_make_coeff_histogram(&cinfo.comp_info[component_index], row_pointers[component_index]);
    s_calculate_distribution_for_rows(&cinfo.comp_info[component_index], row_pointers[component_index]);
    return EXIT_SUCCESS;
}

const char OPTIONS[] = "a:";

int
parse_options(int argc, char *const * argv, std::string & filename, int &teach_value)
{
    int retval = EXIT_SUCCESS;
    int teach_value_internal = 0;

    int option;

    option = getopt(argc, argv, OPTIONS);
    while (option != -1)
    {
        std::string::size_type sz;
        switch (option)
        {
            case 'a':
                teach_value_internal = std::atoi(optarg);
                if (teach_value_internal != -1 && teach_value_internal != 1)
                {
                    std::cerr << "Answer should be 1 for positive and -1 for negative teach result" << std::endl;
                }
                break;
            default:
                retval = EXIT_FAILURE;
        };

        option = (char)getopt(argc, argv, OPTIONS);
    }

    if (retval == EXIT_FAILURE)
    {
        return retval;
    }

    if (optind >= argc)
    {
        std::cerr << "Wrong number of arguments" << std::endl;
    }

    filename = argv[optind];
    teach_value = teach_value_internal;
    std::cout << "File to process: " << filename << std::endl;
    std::cout << "Mode:            " << ((teach_value == 0) ? ("Detect") : ("Learn")) << std::endl;
    if (teach_value != 0)
    {
        std::cout << "Answer:          " << ((teach_value == -1) ? ("Empty container") : ("Filled container")) << std::endl;
    }

    return retval;
}

int
main(int argc, char *const * argv)
{
    std::string file_name;
    int teach_value;

    if (parse_options(argc, argv, file_name, teach_value) != 0)
    {
        return EXIT_FAILURE;
    }

    process_jpeg_file(file_name);

    return EXIT_SUCCESS;
}