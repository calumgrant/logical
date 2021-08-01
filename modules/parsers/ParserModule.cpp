#include "ParserModule.hpp"

#include <filesystem>

void ParserModule::AddLanguage(std::unique_ptr<Language> && language)
{
    languages.push_back(std::move(language));
}

void ParserModule::Parse(Logical::Call & call, const char * filename, const char * language) const
{
    std::filesystem::path p(filename);
    if(std::filesystem::is_regular_file(p))
    {
        ParseFile(call, filename, language);
    }
    else if(std::filesystem::is_directory(p))
    {
        std::vector<std::filesystem::path> filesToParse;
        std::size_t size=0;

        for(auto it = std::filesystem::recursive_directory_iterator(p); it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            auto p = it->path();
            for(auto & lang : languages)
            {
                if(lang->CanParse(*it, language))
                {
                    filesToParse.push_back(p);
                    size += it->file_size();
                    break;
                }
            }
        }
    
        std::cout << "Parsing " << filesToParse.size() << " files in " << p << ", " << size << " bytes total\n";

        int count=0;
        int step = filesToParse.size()/100;
        if(step<1) step=1;
        std::cout << "[";
        for(int i=0; i<filesToParse.size(); i+=step)
            std::cout << "-";
        std::cout << "]\n[";
        
        for(auto & j : filesToParse)
        {
            //std::cout << j << std::endl;
            if(count%step == 0)
            {
                std::cout << ">" << std::flush;
            }
            ++count;
            try
            {
                for(auto & lang : languages)
                {
                    if(lang->CanParse(j, language))
                        lang->ParseFile(j.c_str(), call.GetModule());
                }
            }
            catch(std::bad_alloc&)
            {
                call.Error("Memory limit exceeded");
                throw;
            }
            catch(std::exception &ex)
            {
                call.Error(ex.what());
            }
            catch(...)
            {
                call.Error("Uncaught exception when parsing");
                throw;
            }
        }
        // predicates.Finalize();
        std::cout << "]\n";
    }
}

void ParserModule::ParseFile(Logical::Call & call, const char * filename, const char * language) const
{
    for(auto & lang : languages)
    {
        if(lang->CanParse(filename, language))
            lang->ParseFile(filename, call.GetModule());
    }
}

