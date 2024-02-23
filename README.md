# VVVST

エディタ側：https://github.com/sevenc-nanashi/voicevox/tree/add/vst

## 開発

- エディタをクローンして`npm run vst:serve`すると VST 用のエディタが立ち上がります
- `cmake -S . -B build`でビルドディレクトリを作成して`cmake --build build`でビルドします
- Release ビルドするときはエディタを`npm run vst:build`すると生成される`voicevox.zip`を Resources 内に入れてください

## ライセンス表記

```md
# COx2/audio-plugin-web-ui

Copyright (c) 2023 Tatsuya Shiozawa
Released under the MIT License
https://github.com/COx2/audio-plugin-web-ui/blob/main/LICENSE
```
