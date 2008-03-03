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

#include "RenameTool.h"

#include <OgreBone.h>
#include <OgreMesh.h>
#include <OgreSkeleton.h>
#include <OgreSubMesh.h>

#include "EditableBone.h"
#include "EditableMesh.h"
#include "EditableSkeleton.h"
#include "OgreEnvironment.h"
#include "StatefulMeshSerializer.h"
#include "StatefulSkeletonSerializer.h"

using namespace Ogre;

namespace meshmagick
{
	RenameTool::RenameTool()
	: Tool()
	{
	}

	RenameTool::~RenameTool()
	{
	}

	void RenameTool::doInvoke(
		const OptionList &toolOptions, 
		const Ogre::StringVector &inFileNames, 
		const Ogre::StringVector &outFileNamesArg)
	{
        // Name count has to match, else we have no way to figure out how to apply output
        // names to input files.
        if (!(outFileNamesArg.empty() || inFileNames.size() == outFileNamesArg.size()))
        {
            fail("number of output files must match number of input files.");
        }

        StringVector outFileNames = outFileNamesArg.empty() ? inFileNames : outFileNamesArg;

        // Process the meshes
        for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
        {
            if (StringUtil::endsWith(inFileNames[i], ".mesh", true))
            {
                processMeshFile(toolOptions, inFileNames[i], outFileNames[i]);
            }
            else if (StringUtil::endsWith(inFileNames[i], ".skeleton", true))
            {
                processSkeletonFile(toolOptions, inFileNames[i], outFileNames[i]);
            }
            else
            {
                warn("unrecognised name ending for file " + inFileNames[i]);
                warn("file skipped.");
            }
        }
	}

	void RenameTool::processSkeletonFile(
		const OptionList &toolOptions, Ogre::String inFile, Ogre::String outFile)
    {
        StatefulSkeletonSerializer* skeletonSerializer =
            OgreEnvironment::getSingleton().getSkeletonSerializer();

        print("Loading skeleton " + inFile + "...");
        SkeletonPtr skeleton;
        try
        {
            skeleton = skeletonSerializer->loadSkeleton(inFile);
        }
        catch(std::exception& e)
        {
            warn(e.what());
            warn("Unable to open skeleton file " + inFile);
            warn("file skipped.");
            return;
        }
        print("Processing skeleton...");
		for (OptionList::const_iterator 
			it = toolOptions.begin(); it != toolOptions.end(); ++it)
		{
			if (it->first == "skeleton")
			{
				warn("Skeletons can only be renamed in meshes, skipped skeleton.");
			}
			else if (it->first == "bone")
			{
                // This approach to rename a bone is very brittle.
                // It largely uses the effect, that animations usebone handles instead
                // of their names and that the serializer gets the bones by handle too.
                // The skeleton will only really work again after the serialised version
                // is reloaded, as its lookup-by-name-map is inconsistent as are the entries
                // in this bone's parents' lists.
				StringPair names = split(any_cast<String>(it->second));
				EditableBone* bone = static_cast<EditableBone*>(skeleton->getBone(names.first));
                bone->setName(names.second);
			}
			else if (it->first == "animation")
			{
				StringPair names = split(any_cast<String>(it->second));
				EditableSkeleton* eskel = dynamic_cast<EditableSkeleton*>(skeleton.getPointer());
				Animation* anim = eskel->getAnimation(names.first);
				eskel->removeAnimation(names.first);
				Animation* newAnim = anim->clone(names.second);
				eskel->addAnimation(newAnim);
			}
            else if (it->first == "material")
            {
                warn("Materials can only be renamed in meshes, skipped skeleton.");
            }
		}
        skeletonSerializer->saveSkeleton(outFile, true);
        print("Skeleton saved as " + outFile + ".");
    }

    void RenameTool::processMeshFile(
		const OptionList &toolOptions, Ogre::String inFile, Ogre::String outFile)
    {
        StatefulMeshSerializer* meshSerializer =
            OgreEnvironment::getSingleton().getMeshSerializer();

        print("Loading mesh " + inFile + "...");
        MeshPtr mesh;
        try
        {
            mesh = meshSerializer->loadMesh(inFile);
        }
        catch(std::exception& e)
        {
            warn(e.what());
            warn("Unable to open mesh file " + inFile);
            warn("file skipped.");
            return;
        }
        print("Processing mesh...");

		for (OptionList::const_iterator 
			it = toolOptions.begin(); it != toolOptions.end(); ++it)
		{
			if (it->first == "skeleton")
			{
				mesh->setSkeletonName(any_cast<String>(it->second));
			}
			else if (it->first == "bone")
			{
				warn("Bones can only be renamed in skeletons, skipped mesh.");
			}
			else if (it->first == "animation")
			{
				warn("Animations must be renamed in skeletons, skipped mesh.");
			}
            else if (it->first == "material")
            {
				StringPair names = split(any_cast<String>(it->second));
                String before = names.first;
                String after = names.second;
                for (Mesh::SubMeshIterator it = mesh->getSubMeshIterator();
                    it.hasMoreElements();)
                {
                    SubMesh* submesh = it.getNext();
                    if (submesh->getMaterialName() == before)
                    {
                        submesh->setMaterialName(after);
                    }
                }
            }
            else if (it->first == "submesh")
            {
				StringPair names = split(any_cast<String>(it->second));
                String before = names.first;
                String after = names.second;

                Mesh::SubMeshNameMap smnn = mesh->getSubMeshNameMap();
                Mesh::SubMeshNameMap::iterator it = smnn.find(before);
                EditableMesh* pMesh = dynamic_cast<EditableMesh*>(mesh.getPointer());
                pMesh->renameSubmesh(before, after);
            }
		}
		
		meshSerializer->saveMesh(outFile, true);
        print("Mesh saved as " + outFile + ".");
    }

	RenameTool::StringPair RenameTool::split(const Ogre::String& value) const
	{
        // We expect two string components delimited by '|'
        StringVector components = StringUtil::split(value, ":");
		if (components.size() == 1)
		{
			return make_pair(components[0], components[0]);
		}

		return make_pair(components[0], components[1]);
	}
}
