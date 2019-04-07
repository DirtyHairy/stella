//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"

#include "OSystem.hxx"

#include "FBSurfaceLIBRETRO.hxx"
#include "FrameBufferLIBRETRO.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferLIBRETRO::FrameBufferLIBRETRO(OSystem& osystem)
  : FrameBuffer(osystem),
    myRenderSurface(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferLIBRETRO::queryHardware(vector<GUI::Size>& displays, VariantList& renderers)
{
  displays.emplace_back(1920, 1080);

  VarList::push_back(renderers, "software", "Software");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<FBSurface>
    FrameBufferLIBRETRO::createSurface(uInt32 w, uInt32 h, const uInt32* data) const
{
  unique_ptr<FBSurface> ptr = make_unique<FBSurfaceLIBRETRO>(const_cast<FrameBufferLIBRETRO&>(*this), w, h, data);

  if(w == 565 && h == 320)
  {
    uInt32 pitch;

    ptr.get()->basePtr(myRenderSurface, pitch);
  }

  return ptr;
}
