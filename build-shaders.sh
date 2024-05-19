#!/bin/bash

echo "Compiling shaders..."

vulkanSDKPath="$1"

if [ -z "$vulkanSDKPath" ]; then
    echo "VULKAN_SDK path not provided. Please provide it as an argument."
    exit 1
fi

echo "Assets/shaders/Builtin.MaterialShader.vert.glsl -> Assets/shaders/Builtin.MaterialShader.vert.spv"
$vulkanSDKPath/bin/glslc -fshader-stage=vert Assets/shaders/Builtin.MaterialShader.vert.glsl -o Assets/shaders/Builtin.MaterialShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Assets/shaders/Builtin.MaterialShader.frag.glsl -> Assets/shaders/Builtin.MaterialShader.frag.spv"
$vulkanSDKPath/bin/glslc -fshader-stage=frag Assets/shaders/Builtin.MaterialShader.frag.glsl -o Assets/shaders/Builtin.MaterialShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Assets/shaders/Builtin.UIShader.vert.glsl -> Assets/shaders/Builtin.UIShader.vert.spv"
$vulkanSDKPath/bin/glslc -fshader-stage=vert Assets/shaders/Builtin.UIShader.vert.glsl -o Assets/shaders/Builtin.UIShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Assets/shaders/Builtin.UIShader.frag.glsl -> Assets/shaders/Builtin.UIShader.frag.spv"
$vulkanSDKPath/bin/glslc -fshader-stage=frag Assets/shaders/Builtin.UIShader.frag.glsl -o Assets/shaders/Builtin.UIShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Done."