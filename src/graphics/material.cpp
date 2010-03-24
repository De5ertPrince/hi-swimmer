//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 Steve Baker <sjbaker1@airmail.net>
//                2010 Steve Baker, Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "graphics/material.hpp"

#include <stdexcept>

#include "config/user_config.hpp"
#include "config/stk_config.hpp"
#include "graphics/irr_driver.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "utils/string_utils.hpp"


const unsigned int UCLAMP = 1;
const unsigned int VCLAMP = 2;

//-----------------------------------------------------------------------------
/** Create a new material using the parameters specified in the xml file.
 *  \param node Node containing the parameters for this material.
 *  \param index Index in material_manager.
 */
Material::Material(const XMLNode *node, int index)
{
    
    node->get("name", &m_texname);
    if (m_texname=="")
    {
        throw std::runtime_error("No texture name specified in %s file\n");
    }
    init(index);
    bool b = false;
    node->get("clampU", &b);  if (b) m_clamp_tex |= UCLAMP;
    b = false;
    node->get("clampV", &b);  if (b) m_clamp_tex |= VCLAMP;
    node->get("transparency",     &m_alpha_testing     );
    node->get("lightmap",         &m_lightmap          );
    node->get("alpha",            &m_alpha_blending    );
    node->get("light",            &m_lighting          );
    node->get("sphere",           &m_sphere_map        );
    node->get("friction",         &m_friction          );
    node->get("ignore",           &m_ignore            );
    node->get("zipper",           &m_zipper            );
    node->get("reset",            &m_resetter          );
    node->get("max-speed",        &m_max_speed_fraction);
    node->get("slowdown",         &m_slowdown          );
    node->get("anisotropic",      &m_anisotropic       );
    node->get("backface-culling", &m_backface_culling  );
    std::string s("");
    node->get("graphical-effect", &s                   );
    if(s=="water")
        m_graphical_effect = GE_WATER;
    else if(s=="smoke")
        m_graphical_effect = GE_SMOKE;
    else if (s!="")
        fprintf(stderr, 
            "Invalid graphical effect specification: '%s' - ignored.\n", 
            s.c_str());
    else
        m_graphical_effect = GE_NONE;
    install(/*is_full_path*/false);
}   // Material

//-----------------------------------------------------------------------------
/** Create a standard material using the default settings for materials.
 *  \param fname Name of the texture file.
 *  \param index Unique index in material_manager.
 *  \param is_full_path If the fname contains the full path.
 */
Material::Material(const std::string& fname, int index, bool is_full_path)
{
    m_texname = fname;
    init(index);
    install(is_full_path);
}   // Material

//-----------------------------------------------------------------------------
Material::~Material()
{
}   // ~Material

//-----------------------------------------------------------------------------
void Material::init(unsigned int index)
{
    m_index              = index;
    m_clamp_tex          = 0;
    m_alpha_testing      = false;
    m_lightmap           = false;
    m_alpha_blending     = false;
    m_lighting           = true;
    m_anisotropic        = false;
    m_backface_culling   = true;
    m_sphere_map         = false;
    m_friction           = 1.0f;
    m_ignore             = false;
    m_zipper             = false;
    m_resetter           = false;
    m_max_speed_fraction = 1.0f;
    m_slowdown           = 1.0f;
    m_graphical_effect   = GE_NONE;
}

//-----------------------------------------------------------------------------
void Material::install(bool is_full_path)
{
    // Avoid irrlicht warning about not being able to load texture.
    m_texture = irr_driver->getTexture(file_manager->getTextureFile(m_texname));

    // now set the name to the basename, so that all tests work as expected
    m_texname  = StringUtils::getBasename(m_texname);
}   // install

//-----------------------------------------------------------------------------
/** Sets the appropriate flags in an irrlicht SMaterial.
 *  \param material The irrlicht SMaterial which gets the flags set.
 */
void  Material::setMaterialProperties(video::SMaterial *m) const
{
    if (m_alpha_testing)
    {
        // Note: if EMT_TRANSPARENT_ALPHA_CHANNEL is used, you have to use
        // scene_manager->getParameters()->setAttribute(
        //    scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);  and enable 
        // updates of the Z buffer of the material. Since the _REF 
        // approach is faster (and looks better imho), this is used for now.
        m->MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
    }
    else if (m_alpha_blending)
    {
        m->MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
    }
    else if (m_sphere_map) 
    {
        m->MaterialType = video::EMT_SPHERE_MAP;
    }
    else if (m_lightmap)
    {
        m->MaterialType = video::EMT_LIGHTMAP;
    }

    if (!m_lighting)
    {
        //m->setFlag( video::EMF_LIGHTING, false );
        m->AmbientColor  = video::SColor(255, 255, 255, 255);
        m->DiffuseColor  = video::SColor(255, 255, 255, 255);
        m->EmissiveColor = video::SColor(255, 255, 255, 255);
        m->SpecularColor = video::SColor(255, 255, 255, 255);
    }
    
    if (m_anisotropic && UserConfigParams::m_anisotropic)
    {
        m->setFlag(video::EMF_ANISOTROPIC_FILTER, true);
    }
    else if (UserConfigParams::m_trilinear)
    {
        m->setFlag(video::EMF_TRILINEAR_FILTER, true);
    }
    
#if (IRRLICHT_VERSION_MAJOR == 1) && (IRRLICHT_VERSION_MINOR >= 7)

    if ( (m_clamp_tex & UCLAMP) != 0)
    {
        //  m->setFlag();
        //  n1->getMaterial(0).TextureLayer[0].TextureWrap = video::ETC_CLAMP;
        /**
        //! Texture is clamped to the last pixel
		ETC_CLAMP,
		//! Texture is clamped to the edge pixel
		ETC_CLAMP_TO_EDGE,
		//! Texture is clamped to the border pixel (if exists)
		ETC_CLAMP_TO_BORDER,
        */
        for (unsigned int n=0; n<video::MATERIAL_MAX_TEXTURES; n++)
        {
            m->TextureLayer[n].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
        }
    }
    if ( (m_clamp_tex & VCLAMP) != 0)
    {
        for (unsigned int n=0; n<video::MATERIAL_MAX_TEXTURES; n++)
        {
            m->TextureLayer[n].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
        }
    }

#endif

    if(!m_backface_culling)
        m->setFlag(video::EMF_BACK_FACE_CULLING, false);

}   // setMaterialProperties
