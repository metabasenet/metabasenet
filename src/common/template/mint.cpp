// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mint.h"

#include "block.h"
#include "template.h"

using namespace std;

namespace metabasenet
{

//////////////////////////////
// CTemplateMint

const CTemplateMintPtr CTemplateMint::CreateTemplatePtr(CTemplateMint* ptr)
{
    return boost::dynamic_pointer_cast<CTemplateMint>(CTemplate::CreateTemplatePtr(ptr));
}

} // namespace metabasenet
