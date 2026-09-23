# Portable CAN Driver

μT-Kernel 3.0 環境向けの、再利用可能な小さな CAN ドライバです。
`can_driver.md` の設計方針に従い、CAN 操作・ハードウェア依存・ボード依存を分離しています。

## レイヤ構成

```
Application            アプリ (app_main.c) — 公開 API のみ使用
   │  <can/can.h>
CAN Core               src/can.c — 入力検証・状態管理・ビットタイミング計算
   │  can_hw_ops_t (src/internal/can_hw.h)
Hardware backend       src/hw/stm32h5_fdcan.c — FDCAN レジスタ / Message RAM
   │
Board Support          <board>/Application/can_board.c — GPIO/クロック/インスタンス
```

- 公開ヘッダ `include/can/can.h` は `stm32h5xx.h` や `tk/tkernel.h` に依存しません。
  そのため H5 / H7 の両方、さらには Rust FFI からも同じ API を利用できます。
- CAN Core とバックエンドは関数ポインタテーブル `can_hw_ops_t` で接続します。
  新しいコントローラ（例: STM32H7 FDCAN）は、バックエンド `.c` と ops テーブルを
  追加するだけで対応でき、`can.c` の変更は不要です（Phase 2 で追加予定）。

## Phase 1 の対応範囲

- 対象 MCU: STM32H533 (FDCAN1)
- Classic CAN、標準 11-bit ID、最大 8 バイト
- ビットレート: `can_config_t.nominal_bitrate` から、ボードが渡す
  FDCAN カーネルクロックを用いて自動計算（サンプルポイント約 87.5%）
- 送受信: ポーリング方式・非ブロッキング（`can_try_send` / `can_try_recv`）
- 初期化時の INIT ビット待機には上限あり（ハングしない）

### 未対応（後続フェーズ）

- 拡張 ID / RTR フィルタリング、CAN FD（Phase 3/4）
- 割り込み受信、複数タスク同期、複数インスタンス（Phase 4）
- μT-Kernel デバイス管理 API 統合（Phase 4）

## 使い方（概略）

```c
#include <can/can.h>
#include "can_board.h"

can_config_t cfg = {
    .nominal_bitrate     = 500000,
    .auto_retransmission = true,
    .internal_loopback   = false,
};

can_board_init();                          /* GPIO/クロック */
can_device_t *dev = can_board_get_device();
can_init(dev, &cfg);
can_start(dev);

can_frame_t f = { .id = 0x123, .len = 8, /* ... */ };
can_try_send(dev, &f);

can_frame_t rx;
if (can_try_recv(dev, &rx) == CAN_OK) { /* process */ }
```

## CubeIDE への組み込み

`Components/can/` を各 IDE プロジェクトに **Linked Folder**（名前 `can_driver`）として
登録し、`can_driver/include` と `can_driver/src` を include path に追加、
`can_driver/src` をソースエントリに追加します。
NUCLEO-H533RE プロジェクトでは `.project` / `.cproject` に設定済みです。

## ディレクトリ

```
Components/can/
├── include/can/can.h        公開 API
└── src/
    ├── can.c                CAN Core
    ├── internal/can_hw.h    Core ↔ backend 内部インターフェース
    └── hw/stm32h5_fdcan.c   STM32H5 FDCAN バックエンド
```

ボード固有ファイル `can_board.c` / `can_board.h` は各 IDE プロジェクトの
`Application/` に置きます。
