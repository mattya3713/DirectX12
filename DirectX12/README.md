# DirectX12 学習プロジェクト

C++ / DirectX12 の学習を目的とした個人プロジェクト。過去に作ったゲーム(Senzan)を参考に、PMX/PMDモデルの読み込み・アニメーション・描画パイプラインを一から組んでいる。

設計方針・進捗については`DESIGN.md`を参照。

## コード規約

`C++,DirectXコーディング規約.docx`を正とする。以下はその要約(実装時に見返す用)。

### 文字コード(重要・docx未記載)

このプロジェクトのソースファイルは **UTF-8 BOMあり** で保存する。`.vcxproj`に`/utf-8`コンパイラオプションは設定されていないが、BOM付きUTF-8はコンパイラがBOMを見て自動的にUTF-8と認識するため、フラグの有無に関わらず正しく解釈される(BOMなしUTF-8はShift-JIS/CP932として誤読され、日本語コメントの文字化けや`error C2001`の原因になる)。

- 新規ファイル作成時は、保存後に以下の変換を行うこと(Write系ツールはデフォルトUTF-8 BOMなしで書き出すため):

```powershell
$utf8Bom = New-Object System.Text.UTF8Encoding $true
$text = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
[System.IO.File]::WriteAllText($path, $text, $utf8Bom)
```

- **注意**: Editツールによる部分編集も、対象ファイルがBOMなしだと内部で文字コードを誤判定し日本語コメントが文字化けすることがある(実際に発生した既知の問題)。BOMなしの既存ファイルを編集する場合は、編集後に上記スクリプトでBOM付きUTF-8に変換しておくこと。BOM付きファイルであれば通常この変換は不要。

### 1. コメント規則

著者表記は `mattya3713.`。日付書式は`YYYY/MM/DD`。`TODO`/`FIXME`/`NOTE`は大文字で統一し、可能ならチケット番号を併記する。

**ヘッダー(ファイル内で最初に書く型の直前に1つだけ)**

クラスか構造体かは関係ない。「このヘッダーファイルが何をするか」を書く。1ファイルにつき1つ。

```cpp
/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : ファイルの目的を短く(1〜2行).
*            : 2行目以降はこのように続ける.
**********************************************************************************/
```

**それ以外の構造体・型(同じファイル内の脇役)**

行コメントだけでよい。

```cpp
// 役割を短く書く.
struct Foo { ... };
```

例: `ModelData.h`なら`namespace Model {`の直前に上記ヘッダーを1つ、中の各struct(`Vertex`, `Material`, `Bone`など)の直前にはそれぞれ`// ~~`の一行コメント。

**公開関数(引数複数 / 詳細な説明が要る場合)**

```cpp
/**********************************************************
* @brief                : 処理の概要(1行).
* @param[in] ParamName  : 説明(入力).
* @param[out] OutName   : 説明(出力).
* @return               : 処理結果の意味.
**********************************************************/
```

**関数(引数1つ / なし)**

```cpp
// 更新処理.
void Update();
```

**Set / Get**

```cpp
// Speedの取得.
float GetSpeed() const noexcept;
// Speedの設定.
void SetSpeed(float Speed);
```
- 16byte以下の型: `inline void SetSpeed(float Speed) { m_Speed = Speed; }`
- 16byte超過の型: `inline void SetPosition(const DirectX::XMMATRIX& Position) { m_Position = Position; }`

**メンバ変数**: 行コメントで短く。例: `float m_HP; // プレイヤーの現在HP.`

**列挙値**: 各値に短い説明を付ける。

```cpp
enum class eState {
    Idle, // 待機状態.
    Run   // 走行中.
};
```

**TODO / FIXME / NOTE**: 全て大文字。例: `TODO(JIRA-123): 入力バッファの閾値を調整する`

コメントはすべて日本語。

### 2. 命名規則

**基本命名**

| 種類 | 規則 |
|---|---|
| クラス | PascalCase |
| ファイル(.h) | クラス名と一致 |
| 関数 | PascalCase |
| 関数引数 | メンバ変数の`m_`を除いたPascalCase |
| 通常変数 | PascalCase |
| ローカル変数 | snake_case |
| ローカルポインタ | `p_` + snake_case |
| constローカルポインタ | `cp_` + snake_case |
| DirectX::XMVECTORローカル | `v_` + snake_case |

**メンバ変数接頭辞(クラス)**

| 種類 | 規則 | 例 |
|---|---|---|
| 通常 | `m_` + PascalCase | `m_Speed` |
| 生ポインタ | `m_p` + PascalCase | `m_pDevice` |
| unique_ptr | `m_up` + PascalCase | `m_upBuffer` |
| shared_ptr | `m_sp` + PascalCase | `m_spTexture` |
| weak_ptr | `m_wp` + PascalCase | `m_wpParent` |
| static | `s_` + PascalCase | `s_InstanceCount` |
| bool | `m_Is` + PascalCase | `m_IsActive` |
| 配列 | `m_` + PascalCase + `s` | `m_Values` |
| MyComPtr | `m_cp` + PascalCase | `m_cpVS` |

**構造体メンバ**: プレフィックスなしPascalCase(クラスメンバの`m_`と明確に区別)。

**関数引数**

| 種類 | 規則 |
|---|---|
| 出力引数 | `Out` + PascalCase |
| その他 | PascalCase |

例: `bool RayCast(const Ray& Ray, HitResult& OutHit);`

**bool規則(強制)**
- メンバ変数: `m_Is` + PascalCase
- 関数: `bool IsPascalCase() const`
- 状態を変更しないbool戻り値関数は必ずconst
- 禁止: `Get` / `Check` / `Should`

**その他命名**

| 種類 | 規則 |
|---|---|
| 定数(.cpp) | 全大文字 + `_` |
| グローバル | `g_` + PascalCase |
| typedef / using | PascalCase |
| マクロ | 全大文字 + `_` |
| Interface | `I` + PascalCase |
| 抽象基底 | PascalCase + `Base` |

### 3. クラス設計規則
- すべてのクラスは「virtualデストラクタを持つ(継承前提)」か「finalを付ける」のいずれか。曖昧な拡張可能クラスは禁止。
- Base命名条件: 他クラスの基底になり、自身は実体にならない(抽象クラス)。
- Interfaceは`I`プレフィックス必須、純粋仮想のみ、メンバ変数禁止、virtualデストラクタ必須。

```cpp
class IRenderable
{
public:
    virtual ~IRenderable() = default;
    virtual void Draw() = 0;
};
```

### 4. コピー・ムーブ規則
- 所有クラスはコピー禁止。ムーブ可能なら明示。ムーブは必ずnoexcept。仮想関数を持つクラスはvirtualデストラクタ必須。

```cpp
class Texture final
{
public:
    Texture() = default;
    ~Texture() noexcept = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;
};
```

### 5. explicit規則
- 単一引数コンストラクタは必ず`explicit`(コピー/ムーブは除外)。暗黙変換を許可する場合は理由コメント必須。

### 6. const規則
- メンバ関数: 取得系は必ずconst、bool判定関数もconst、getterはnoexcept。
- 引数: 16byte以下は値渡し、16byte超過は`const T&`。
- ローカル: 再代入しない変数はconst。

### 7. noexcept規則
- ムーブコンストラクタ・デストラクタ・単純getterは必ずnoexcept。

### 8. enum規則
- 基本`enum class`。サイズ明示(例: `: uint8_t`)。ビットフラグは`1 << n`。

### 9. namespace規則
- グローバル空間では原則使用しない(State系など内部整理用途のみ可)。
- `using namespace`禁止。DirectXは常にフルネーム記述(`DirectX::XMVECTOR`)。

### 10. インデント・括弧
- タブインデント(4相当)。短い処理は同一行可(`if (!p_device) { return false; }`)。通常処理は改行。

### 11. アクセス指定子順
```
public:    // struct / enum
public:    // 外部API
protected: // 派生用関数
private:   // 内部処理
private:   // メンバ変数
```

### 12. ファイル規則
- 1ファイル1クラス。`#pragma once`必須。
- include順: 自クラスヘッダ → プロジェクト内ヘッダ → サードパーティ(DirectX等) → 標準ライブラリ。
- 例外: 「データ専用ヘッダー」(`ModelData.h`のように、関連するPOD構造体をまとめて置くファイル)は複数structをまとめてよい。同様に、あるフォーマットの生バイナリ構造体群(`namespace PMD { struct Header; ... }`)とそれを読む専用パーサークラス(`PMDParser`)を同じファイルに置くのも許容する。

### 13. 実装・リソース管理
- デストラクタ: nullptr代入禁止、RAII徹底、raw所有禁止。DirectXリソースは`MyComPtr`推奨。
- 所有: 単独所有は`unique_ptr`、共有は`shared_ptr`、参照のみはrawポインタ。
- `mutable`: 原則禁止。キャッシュ用途のみ可、使用時はコメント必須。
- `friend`: 原則禁止。強い内部結合時のみ許可、class単位で指定。

### 14. メンバ初期化
- コンストラクタの初期化子リストは`()`ではなく`{}`(波括弧初期化)を使う。`{}`は縮小変換(narrowing conversion、例: `float x{3.14159265...}`のような精度欠落)をコンパイルエラーで検出できる。
- 例外: `std::vector`など`initializer_list`コンストラクタを持つ型は`()`と`{}`で挙動が変わる場合がある(`vector<int> v(3, 5)`は`{5,5,5}`、`v{3, 5}`は`{3, 5}`)。該当する型を初期化する際は意図した挙動になっているか確認すること。`DirectX::XMMATRIX`/`XMFLOAT3`などのPOD/SIMD型にはこの問題はない。

### 毎フレーム処理禁止事項
- `new` / `malloc`禁止。`push_back`(リサイズ発生)禁止。

### 将来的にやりたいこと(未導入)
- CIにDoxygenを導入する場合は`@brief`/`@param`コメントのテンプレを満たすこと。
- pre-commitフックでファイルヘッダ・公開関数の`@brief`存在チェック。
- 新規ファイルは規約必須、既存コードは段階的に適用(PRでコメント追加を必須にする運用)。
