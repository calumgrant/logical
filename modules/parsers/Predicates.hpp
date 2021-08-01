
#pragma once
#include "Logical.hpp"

struct Predicates
{
    Predicates(Logical::Module & module) :
        file_filename_language(module.GetPredicate({"parsers:file","filename","language"})),
        file_errormessage(module.GetPredicate({"parsers:file", "errormessage"})),
        token_text(module.GetPredicate({"parsers:token","text"})),
        node_type_parent_index_location_language(module.GetPredicate({"parsers:node","type","parent","index","location","language"})),
        location_filename_startrow_startcol_endrow_endcol(module.GetPredicate({"parsers:location","filename","startrow","startcol","endrow","endcol"}))
    {
    }
    
    void Finalize()
    {
        file_filename_language.Finalize();
        file_errormessage.Finalize();
        token_text.Finalize();
        node_type_parent_index_location_language.Finalize();
        node_type_parent_index_location_language.Finalize();
        location_filename_startrow_startcol_endrow_endcol.Finalize();
    }

    Logical::Call & file_filename_language;
    Logical::Call & file_errormessage;
    Logical::Call & token_text;
    Logical::Call & node_type_parent_index_location_language;
    Logical::Call & location_filename_startrow_startcol_endrow_endcol;
};
