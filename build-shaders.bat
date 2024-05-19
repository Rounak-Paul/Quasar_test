@echo off
setlocal enabledelayedexpansion

echo "Compiling shaders..."

set vulkanSDKPath=%1

if "%vulkanSDKPath%"=="" (
    echo VULKAN_SDK path not provided. Please provide it as an argument.
    exit /b 1
)

echo "Assets/shaders/Builtin.MaterialShader.vert.glsl -> Assets/shaders/Builtin.MaterialShader.vert.spv"
%vulkanSDKPath%\bin\glslc.exe -fshader-stage=vert Assets/shaders/Builtin.MaterialShader.vert.glsl -o Assets/shaders/Builtin.MaterialShader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Assets/shaders/Builtin.MaterialShader.frag.glsl -> Assets/shaders/Builtin.MaterialShader.frag.spv"
%vulkanSDKPath%\bin\glslc.exe -fshader-stage=frag Assets/shaders/Builtin.MaterialShader.frag.glsl -o Assets/shaders/Builtin.MaterialShader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Assets/shaders/Builtin.UIShader.vert.glsl -> Assets/shaders/Builtin.UIShader.vert.spv"
%vulkanSDKPath%\bin\glslc.exe -fshader-stage=vert Assets/shaders/Builtin.UIShader.vert.glsl -o Assets/shaders/Builtin.UIShader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Assets/shaders/Builtin.UIShader.frag.glsl -> Assets/shaders/Builtin.UIShader.frag.spv"
%vulkanSDKPath%\bin\glslc.exe -fshader-stage=frag Assets/shaders/Builtin.UIShader.frag.glsl -o Assets/shaders/Builtin.UIShader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Done."