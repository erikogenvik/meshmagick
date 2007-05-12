/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007 - Daniel Wickert

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StatefulMeshSerializer.h"

#include <OgreResourceGroupManager.h>
#include <OgreMeshManager.h>

#include <iostream>

using namespace Ogre;

namespace meshmagick
{
    const unsigned short HEADER_CHUNK_ID = 0x1000;

    MeshPtr StatefulMeshSerializer::loadMesh(const String& name)
    {
        mMesh = MeshManager::getSingleton().create(name, 
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        std::ifstream ifs;
        ifs.open(name.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!ifs)
        {
            throw std::exception(("cannot open file " + name).c_str());
        }

        DataStreamPtr stream(new FileStreamDataStream(name, &ifs, false));

        determineFileFormat(stream);

        importMesh(stream, mMesh.getPointer());

        ifs.close();

        return mMesh;
    }

    void StatefulMeshSerializer::saveMesh(const Ogre::String& name,
        bool keepVersion, bool keepEndianess)
    {
        if (mMesh.isNull())
        {
            throw std::exception("No mesh to save set.");
        }

        String version = keepVersion ? mMeshFileVersion : msCurrentVersion;
        Endian endianMode = keepEndianess ? mMeshFileEndian : ENDIAN_NATIVE;

        /// HACK. In order to save in the correct file version, we have to rig msCurrentVersion.
        String oldVersion = msCurrentVersion;
        msCurrentVersion = version;
        exportMesh(mMesh.getPointer(), name, endianMode);
        msCurrentVersion = oldVersion;
    }

    void StatefulMeshSerializer::clear()
    {
        mMesh.setNull();
        mMeshFileEndian = ENDIAN_NATIVE;
        mMeshFileVersion = "";
    }

    MeshPtr StatefulMeshSerializer::getMesh() const
    {
        return mMesh;
    }

    void StatefulMeshSerializer::determineFileFormat(DataStreamPtr stream)
    {
        determineEndianness(stream);

        // Read header and determine the version
        unsigned short headerID;
        
        // Read header ID
        readShorts(stream, &headerID, 1);
        
        if (headerID != HEADER_CHUNK_ID)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "File header not found",
                "MeshSerializer::importMesh");
        }
        // Read version
        mMeshFileVersion = readString(stream);
        // Jump back to start
        stream->seek(0);

#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        mMeshFileEndian = mFlipEndian ? ENDIAN_LITTLE : ENDIAN_BIG;
#else
        mMeshFileEndian = mFlipEndian ? ENDIAN_BIG : ENDIAN_LITTLE;
#endif
    }

    Ogre::String StatefulMeshSerializer::getMeshFileVersion() const
    {
        return mMeshFileVersion;
    }

    Ogre::Serializer::Endian StatefulMeshSerializer::getEndianMode() const
    {
        return mMeshFileEndian;
    }
}
