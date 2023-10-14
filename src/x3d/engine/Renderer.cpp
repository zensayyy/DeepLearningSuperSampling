#include "Renderer.hpp"
#include "../world/Nodes.hpp"
#include "Frame.hpp"
#include "Metal.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <ostream>
#include <tuple>
#include <vector>

using namespace x3d::engine;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

#define FRAME_AVG 30
#define TRIPPLE_FRAME_BUFFERING 3

using namespace x3d::mesh;

Renderer::Renderer(MTL::Device *device, MTK::View *view)
    : MTK::ViewDelegate(), pDevice(NS::TransferPtr(device)), fps(FRAME_AVG),
      frame(0), view(NS::RetainPtr(view)),
      frameBuffer(TRIPPLE_FRAME_BUFFERING) {
  pCommandQueue = NS::TransferPtr(pDevice->newCommandQueue());
  std::vector<NS::SharedPtr<MTL::Buffer>> trippleframeBuffers = {
      NS::TransferPtr(pDevice->newBuffer(sizeof(Frame), {})),
      NS::TransferPtr(pDevice->newBuffer(sizeof(Frame), {})),
      NS::TransferPtr(pDevice->newBuffer(sizeof(Frame), {})),
  };
  if (pDevice.get() == nullptr) {
    std::cout << "[Renderer::Renderer] creating device failed" << std::endl;
    exit(1);
  }

  frameBuffer.manageThis(std::move(trippleframeBuffers));

  pLibrary = NS::TransferPtr(mesh::Mesh::BuildShaders(pDevice.get()));
}

Renderer::~Renderer() {}

void Renderer::drawInMTKView(MTK::View *pView) {
  auto now =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  fps.update(now.count());
  frame++;

  NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();

  MTL::CommandBuffer *pCmd = pCommandQueue->commandBuffer();
  MTL::RenderPassDescriptor *pRpd = pView->currentRenderPassDescriptor();

  pRpd->colorAttachments()->object(0)->setLoadAction(
      MTL::LoadAction::LoadActionClear);
  pRpd->colorAttachments()->object(0)->setClearColor(scene.clearColor);

  MTL::RenderCommandEncoder *pEnc = pCmd->renderCommandEncoder(pRpd);
  scene.renderScene(pEnc);
  pEnc->endEncoding();
  pCmd->presentDrawable(pView->currentDrawable());
  pCmd->commit();

  pPool->release();

  if (frame % FRAME_AVG == 0) {
    std::cout << "\r"
              << "FPS: " << fps.getFPS() << std::flush;
  }
}

std::tuple<MTL::PixelFormat, MTL::PixelFormat> Renderer::getPixelFormat() {
  return std::make_tuple(view->colorPixelFormat(),
                         view->depthStencilPixelFormat());
}
