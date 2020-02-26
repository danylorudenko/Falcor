/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "ProjectTemplate.h"

void ProjectTemplate::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
    pGui->addText("Hello from ProjectTemplate");
    if (pGui->addButton("Click Here"))
    {
        msgBox("Now why would you do that?");
    }

    if (pGui->addButton("Load Model"))
    {
        std::string fileName;
        FileDialogFilterVec filters;
        if (openFileDialog(Model::kFileExtensionFilters, fileName))
        {
            loadModelFromFile(fileName);
        }
    }
}

void ProjectTemplate::onLoad(SampleCallbacks* pSample, RenderContext* pRenderContext)
{
    m_Camera = Camera::create();
    m_CameraController.attachCamera(m_Camera);

    m_GraphicsState = GraphicsState::create();

    RasterizerState::Desc rastDesc;
    rastDesc.setCullMode(RasterizerState::CullMode::Back);
    m_ResterizerState = RasterizerState::create(rastDesc);

    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthTest(true);
    dsDesc.setDepthFunc(DepthStencilState::Func::LessEqual);
    dsDesc.setDepthWriteMask(true);
    m_DepthStencilState = DepthStencilState::create(dsDesc);

    m_GraphicsProgram = GraphicsProgram::createFromFile("SimpleShader.ps.hlsl", "", "main");
    m_GraphicsVars = GraphicsVars::create(m_GraphicsProgram->getReflector());
}

void ProjectTemplate::loadModelFromFile(std::string const& fileName)
{
    Model::LoadFlags flags = Model::LoadFlags::None;
    m_TestModel = Model::createFromFile(fileName.c_str(), flags);
    if (!m_TestModel)
    {
        msgBox("Failed to load model");
        return;
    }
}

void ProjectTemplate::onFrameRender(SampleCallbacks* pSample, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    m_Camera->beginFrame();

    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if (m_TestModel)
    {
        m_GraphicsState->setFbo(pTargetFbo);
        m_GraphicsState->setProgram(m_GraphicsProgram);
        m_GraphicsState->setRasterizerState(m_ResterizerState);
        m_GraphicsState->setDepthStencilState(m_DepthStencilState);

        pRenderContext->setGraphicsState(m_GraphicsState);
        pRenderContext->setGraphicsVars(m_GraphicsVars);

        ModelRenderer::render(pRenderContext, m_TestModel, m_Camera.get());
    }
}

void ProjectTemplate::onShutdown(SampleCallbacks* pSample)
{
}

bool ProjectTemplate::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
    // hehe, boiiiiiiii
    //m_Camera->setPosition();
    return m_CameraController.onKeyEvent(keyEvent);

}

bool ProjectTemplate::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
    return m_CameraController.onMouseEvent(mouseEvent);
}

void ProjectTemplate::onDataReload(SampleCallbacks* pSample)
{

}

void ProjectTemplate::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ProjectTemplate::UniquePtr pRenderer = std::make_unique<ProjectTemplate>();
    SampleConfig config;
    config.windowDesc.title = "drudenko project";
    config.windowDesc.resizableWindow = true;
    Sample::run(config, pRenderer);
    return 0;
}
