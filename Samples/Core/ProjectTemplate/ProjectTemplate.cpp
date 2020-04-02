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

#include "GlobalConstants.h"

ProjectTemplate::ProjectTemplate()
    : Renderer{}
    , m_IsFirstPerson{ true }
{}

static int enableTonemapping = true;

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

    if (pGui->addCheckBox("Enable FirstPerson Camera", m_IsFirstPerson))
    {
        resetCamera();
    }

    pGui->addSeparator();

    if (pGui->beginGroup("TonemapParams", true))
    {
        pGui->addCheckBox("Enable tonemapping", enableTonemapping);

        pGui->addFloatVar("P", m_TonemapParams.P);
        pGui->addFloatVar("a", m_TonemapParams.a);
        pGui->addFloatVar("m", m_TonemapParams.m);
        pGui->addFloatVar("l", m_TonemapParams.l);
        pGui->addFloatVar("c", m_TonemapParams.c);
        pGui->addFloatVar("b", m_TonemapParams.b);
        pGui->endGroup();
    }
}

void ProjectTemplate::onLoad(SampleCallbacks* pSample, RenderContext* pRenderContext)
{
    m_Camera = Camera::create();

    m_FPCameraController.attachCamera(m_Camera);
    m_MVCameraController.attachCamera(m_Camera);

    m_DirectionalLight = DirectionalLight::create();
    m_DirectionalLight->setWorldDirection({0.0f, 0.0f, 1.0f});
    m_DirectionalLight->setIntensity({1.0f, 1.0f, 1.0f});
    m_DirectionalLight->setWorldParams({0.0f, 0.0f, 0.0f}, 1000.0f);

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

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    m_Sampler = Sampler::create(samplerDesc);

    m_SkyBox = SkyBox::create("J:\\Assets\\SunTemple_v3\\SunTemple\\SunTemple_Skybox.hdr", true, m_Sampler);

    m_MainScene = Scene::create("MainScene");
    m_MainSceneRenderer = SceneRenderer::create(m_MainScene);
    m_MainSceneRenderer->toggleMeshCulling(true);

    loadModelFromFile("J:\\Assets\\SunTemple_v3\\SunTemple\\SunTemple.fbx");

    m_MainRenderTargetTexture = Texture::create2D(
        C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y,
        ResourceFormat::RGBA16Float, 1, 1, nullptr,
        Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource
    );

    m_MainRenderTargetDepth = Texture::create2D(
        C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y,
        ResourceFormat::D16Unorm, 1, 1, nullptr,
        Resource::BindFlags::DepthStencil
    );

    //m_MainRenderTargetView = m_MainRenderTargetTexture->getRTV();

    m_MainFbo = Fbo::create();
    m_MainFbo->attachColorTarget(m_MainRenderTargetTexture, 0, 0, 0, 1);
    m_MainFbo->attachDepthStencilTarget(m_MainRenderTargetDepth, 0, 0, 1);

    m_TonemapTargetTexture = Texture::create2D(
        C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y,
        ResourceFormat::RGBA8Unorm, 1, 1, nullptr,
        Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource
    );
    m_TonemapProgram = ComputeProgram::createFromFile("Tonemap.hlsl", "mainTonemap");
    m_TonemapState = ComputeState::create();
    m_TonemapState->setProgram(m_TonemapProgram);
    m_TonemapVars = ComputeVars::create(m_TonemapProgram->getReflector());

    m_TonemapLUT = Texture::create1D(128, ResourceFormat::R8Unorm, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess);
    m_TonemapGenProgram = ComputeProgram::createFromFile("Tonemap.hlsl", "mainGenLUT");
    m_TonemapGenState = ComputeState::create();
    m_TonemapGenState->setProgram(m_TonemapGenProgram);
    m_TonemapGenVars = ComputeVars::create(m_TonemapGenProgram->getReflector());


    m_TonemapGenVars->setTexture("g_TonemapLUT", m_TonemapLUT);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("PMem", m_TonemapParams.P);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("aMem", m_TonemapParams.a);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("mMem", m_TonemapParams.m);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("lMem", m_TonemapParams.l);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("cMem", m_TonemapParams.c);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("bMem", m_TonemapParams.b);
    m_TonemapGenVars["TonemapParamsAlt"]->setVariable("enableTonemapping", enableTonemapping);
    pRenderContext->setComputeState(m_TonemapGenState);
    pRenderContext->setComputeVars(m_TonemapGenVars);
    pRenderContext->dispatch(4, 1, 1);
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

    m_TestModel->bindSamplerToMaterials(m_Sampler);
    resetCamera();

    m_MainScene->addModelInstance(m_TestModel, "MainModelInstance");

    m_Camera->setPosition({ 6.54264307f, 6.81390476f, 4.28869581f });
    m_Camera->setTarget({ 5.84925079f, 6.79420948f, 3.56840467f });
}

void ProjectTemplate::onFrameRender(SampleCallbacks* pSample, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    m_Camera->beginFrame();

    const glm::vec4 clearColor(0.1f, 0.1f, 0.1f, 1);
    pRenderContext->clearFbo(m_MainFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if (m_TestModel)
    {
        if (m_IsFirstPerson)
        {
            m_FPCameraController.update();

        }
        else
        {
            m_MVCameraController.update();
        }

        m_GraphicsState->setFbo(m_MainFbo);
        m_GraphicsState->setProgram(m_GraphicsProgram);
        m_GraphicsState->setRasterizerState(m_ResterizerState);
        m_GraphicsState->setDepthStencilState(m_DepthStencilState);

        m_SkyBox->render(pRenderContext, m_Camera.get(), m_MainFbo);

        pRenderContext->setGraphicsState(m_GraphicsState);

        m_DirectionalLight->setIntoProgramVars(m_GraphicsVars.get(), m_GraphicsVars["PerFrameCB"].get(), "gDirLight");
        m_GraphicsVars["PerFrameCB"]["gAmbient"] = glm::vec3{ 0.2f, 0.2f, 0.2f };
        pRenderContext->setGraphicsVars(m_GraphicsVars);

        m_MainSceneRenderer->renderScene(pRenderContext, m_Camera.get());
    }

    ////////////////////////
    // for debugging we'll generate TonemapLUT each frame
    //m_TonemapGenVars->setTexture("g_TonemapLUT", m_TonemapLUT);
    //m_TonemapGenVars["TonemapParamsAlt"]["PMem"] = m_TonemapParams.P;
    //m_TonemapGenVars["TonemapParamsAlt"]["aMem"] = m_TonemapParams.a;
    //m_TonemapGenVars["TonemapParamsAlt"]["mMem"] = m_TonemapParams.m;
    //m_TonemapGenVars["TonemapParamsAlt"]["lMem"] = m_TonemapParams.l;
    //m_TonemapGenVars["TonemapParamsAlt"]["cMem"] = m_TonemapParams.c;
    //m_TonemapGenVars["TonemapParamsAlt"]["bMem"] = m_TonemapParams.b;
    //m_TonemapGenVars["TonemapParamsAlt"]->setVariable("enableTonemapping", enableTonemapping);
    pRenderContext->setComputeVars(m_TonemapGenVars);
    pRenderContext->setComputeState(m_TonemapGenState);
    pRenderContext->dispatch(4, 1, 1);
    // end
    ////////////////////////


    m_TonemapVars->setTexture("g_InputTexture", m_MainRenderTargetTexture);
    m_TonemapVars->setTexture("g_OutputTexture", m_TonemapTargetTexture);
    m_TonemapVars->setTexture("g_TonemapLUT", m_TonemapLUT);
    pRenderContext->setComputeState(m_TonemapState);
    pRenderContext->setComputeVars(m_TonemapVars);
    std::uint32_t const xDim = C_RENDER_TARGET_RESOLUTION_X / 16 + 1;
    std::uint32_t const yDim = C_RENDER_TARGET_RESOLUTION_Y / 16 + 1;
    pRenderContext->dispatch(xDim, yDim, 1);

    pRenderContext->blit(
        m_TonemapTargetTexture->getSRV(), pTargetFbo->getColorTexture(0)->getRTV(),
        uvec4{ 0, 0, C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y },
        uvec4{ 0, 0, C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y },
        Sampler::Filter::Point
    );
}

void ProjectTemplate::onShutdown(SampleCallbacks* pSample)
{
}

bool ProjectTemplate::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
    if (m_IsFirstPerson)
    {
        return m_FPCameraController.onKeyEvent(keyEvent);
    }
    else
    {
        return m_MVCameraController.onKeyEvent(keyEvent);
    }
}

bool ProjectTemplate::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
    if (m_IsFirstPerson)
    {
        return m_FPCameraController.onMouseEvent(mouseEvent);
    }
    else
    {
        return m_MVCameraController.onMouseEvent(mouseEvent);
    }
}

void ProjectTemplate::onDataReload(SampleCallbacks* pSample)
{

}

void ProjectTemplate::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    float h = (float)height;
    float w = (float)width;

    m_Camera->setFocalLength(21.0f);
    float aspectRatio = (w / h);
    m_Camera->setAspectRatio(aspectRatio);
    m_Camera->setDepthRange(0.1f, 1000.0f);
}

void ProjectTemplate::resetCamera()
{
    if (m_TestModel)
    {
        // update the camera position
        float radius = m_TestModel->getRadius();
        const glm::vec3& modelCenter = m_TestModel->getCenter();
        glm::vec3 camPos = modelCenter;
        camPos.z += radius;

        m_Camera->setPosition(camPos);
        m_Camera->setTarget(modelCenter);
        m_Camera->setUpVector(glm::vec3(0, 1, 0));

        // Update the controllers
        m_MVCameraController.setModelParams(modelCenter, radius, 3.5f);
        m_FPCameraController.setCameraSpeed(radius * 0.25f);
    }
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ProjectTemplate::UniquePtr pRenderer = std::make_unique<ProjectTemplate>();
    SampleConfig config;
    config.windowDesc.title = "drudenko project";
    config.windowDesc.resizableWindow = false;
    config.windowDesc.width = C_RENDER_TARGET_RESOLUTION_X;
    config.windowDesc.height = C_RENDER_TARGET_RESOLUTION_Y;
    Sample::run(config, pRenderer);
    return 0;
}
