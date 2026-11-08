# 翻訳テーブルマージ機構

## 概要

BooruPrompterでは、上流の機械翻訳テーブル（`external/booru-japanese-tag/danbooru-machine-jp.csv`）を保持しつつ、独自の修正・追加翻訳を管理できるビルド時マージ機構を実装しています。

## 仕組み

ビルド時に以下の処理が自動実行されます：

1. マージツール（`merge_translation.exe`）がビルドされる
2. 元テーブル + 修正テーブル → 完成テーブルをマージ生成
3. 完成テーブル（`bin/danbooru-machine-jp.csv`）をアプリが読み込む

## ファイル構成

```
src/
  translation_overrides.csv      独自の修正・追加翻訳（編集対象）
  tools/
    merge_translation.cpp        マージツール本体
    merge_translation.vcxproj    マージツールのビルド設定

external/
  booru-japanese-tag/
    danbooru-machine-jp.csv      上流の元テーブル（編集不可）

bin/
  danbooru-machine-jp.csv        生成される完成テーブル（編集不可）
```

## 使い方

### 1. 翻訳を修正・追加する

`src/translation_overrides.csv` をテキストエディタで編集します。

**形式**: タグ名,日本語訳

**例**:
```csv
1girl,一人の女の子
blue_eyes,青い瞳
my_custom_tag,私のカスタムタグ
```

**注意事項**:
- タグ名をキーとして、このファイルの訳が元テーブルより優先されます
- 元テーブルにないタグも追加できます
- 同じタグ名を複数回記述するとビルドエラーになります
- カンマや引用符を含む場合は `"..."` で囲んでください
- ファイルは **UTF-8 BOM付き** で保存してください
- コメント行は `#` で始めてください

### 2. ビルドする

Visual Studioで通常通りビルドすると、自動的にマージが実行されます。

ビルド出力に以下のようなメッセージが表示されます：

```
Building translation merge tool...
Merging translation tables...
Translation CSV Merge Tool
===========================
Base file:     C:\...\external\booru-japanese-tag\danbooru-machine-jp.csv
Override file: C:\...\src\translation_overrides.csv
Output file:   C:\...\bin\danbooru-machine-jp.csv

Loaded 100000 entries from base table.
Loaded 3 entries from override table.

Merge summary:
  Base entries:      100000
  Override entries:  3
  - Overwrites:      2
  - New additions:   1
  Total entries:     100001

Successfully merged translation tables to: C:\...\bin\danbooru-machine-jp.csv
```

### 3. 確認する

アプリを起動し、修正した翻訳が反映されていることを確認してください。

## エラー処理

### 重複キーエラー

同一ファイル内で同じタグ名が複数回記述されている場合、ビルドが失敗します。

**エラー例**:
```
ERROR: Duplicate tag in translation_overrides.csv at line 5: blue_eyes
  First occurrence at line 3
```

**対処法**: 重複したタグ名を削除または統合してください。

### ファイルエンコーディングエラー

UTF-8以外のエンコーディングで保存すると、日本語が正しく処理されない可能性があります。

**対処法**: ファイルを **UTF-8 BOM付き** で保存し直してください。

## 手動マージ

コマンドラインから手動でマージすることも可能です：

```cmd
cd bin\tools
merge_translation.exe ^
  ..\..\external\booru-japanese-tag\danbooru-machine-jp.csv ^
  ..\..\src\translation_overrides.csv ^
  ..\danbooru-machine-jp.csv
```

## テスト

マージ機構のテストは `test/TranslationMergeTest.cpp` に実装されています。

Visual Studioの Test Explorer から実行できます：

- TestNormalMerge: 通常のマージ
- TestOverwrite: 上書き
- TestNewAddition: 新規追加
- TestJapaneseCharacters: 日本語文字
- TestQuotedFields: 引用符付きフィールド
- TestCommaInField: カンマを含むフィールド
- TestDuplicateInBase: 元テーブル内の重複検出
- TestDuplicateInOverride: 修正テーブル内の重複検出
- TestEmptyOverride: 空の修正テーブル
- TestSortedOutput: ソート済み出力

## ライセンス

元テーブル（`external/booru-japanese-tag/danbooru-machine-jp.csv`）は MIT License です。
詳細は `external/booru-japanese-tag/LICENSE` を参照してください。

独自の修正テーブルはBooruPrompterと同じライセンスで管理されます。
