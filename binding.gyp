{
    "targets": [
        {
            "target_name": "AudioEndpoints",
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "AdditionalOptions": [
                        "-std:c++20",
                    ],
                },
            },
            "sources": [
                "getAudioEndpoints.cpp",
                "readWav.cpp",
                "Source.cpp",
                "mouseListener.cpp",
                "TextToSpeechKeyboardListener.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
            ],
            "libraries": [],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
        }
    ]
}
