#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    std::ofstream out(argv[argc - 1], std::ofstream::trunc);

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " infile(s) outfile" << std::endl;
        return EXIT_FAILURE;
    }

    out << "#include <map>" << std::endl;
    out << "#include <string>" << std::endl;
    out << "std::map<std::string_view, const unsigned char*> transitions_map = {" << std::endl;

    for (int i = 1; i < argc - 1; ++i)
    {
        std::ifstream in{ argv[i] };

        if (!in.is_open())
        {
            std::cerr << "Input file '" << argv[i] << "' it doesn't exist." << std::endl;
            return EXIT_FAILURE;
        }

        out << "{" << std::endl;
        std::string name{ argv[i] };
        name = name.substr(name.find_last_of('/') + 1);
        out << "\"" << name.substr(0, name.find_last_of('.')) << "\", new unsigned char[]{"
            << std::endl;

        size_t line_count{ 0 };
        in.seekg(0, in.beg);

        while (!in.eof())
        {
            char c;
            in.get(c);

            out << "0x" << std::hex << (c & 0xff) << ",";
            if (++line_count == 10)
            {
                line_count = 0;
                out << std::endl;
            }
        }
        out << "} }," << std::endl;
    }
    out << std::endl << "};" << std::endl;

    return EXIT_SUCCESS;
}
