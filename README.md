# AQUOS wish3 (SH-53D) Open Source Software

シャープが GPL / LGPL に基づき公開している AQUOS wish3 のオープンソースソフトウェアを、
配布された古いビルドから順にコミットし、ビルド番号でタグ付けしたリポジトリです。

配布元: https://k-tai.sharp.co.jp/support/developers/oss/aquos-wish3/index.html

## タグ

| タグ | ビルド番号 | 配布ファイル | 公開日 |
| --- | --- | --- | --- |
| `V1.20A` | V1.20A | `AQUOS_wish3_CMN_V1_20A.7z` | 2024/02/07 |
| `V2.23A` | V2.23A | `AQUOS_wish3_CMN_2_23A.7z`  | 2024/12/25 |
| `V3.21G` | V3.21G | `AQUOS_wish3_CMN_3_21G.7z`  | 2025/07/08 |

各タグは配布アーカイブを展開した内容そのものです（アーカイブ最上位のディレクトリは剥がしています）。

## 除外ファイル

GitHub の 1 ファイル 100 MB 制限を超えるため、以下のビルド済みカーネルバイナリは
リポジトリに含めていません（V1.20A / V2.23A に存在）。

- `LINUX/android/kernel/prebuilts/5.10/arm64/vmlinux` (462 MB)
- `LINUX/android/kernel/prebuilts/5.10/arm64/vmlinux-allsyms` (465 MB)

これらはソースではなくビルド成果物です。オリジナルの配布アーカイブは上記配布元から取得できます。

## ライセンス

各ソースツリー内の COPYING / LICENSE 等に従います。
GPL v2.0 / LGPL v2.1 に基づき "現状のまま"、"無保証" で提供されるものです。
