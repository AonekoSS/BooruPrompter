# About
[Danbooru](https://danbooru.donmai.us/)のタグを日本語化するためのリポジトリです

全てを日本語化することは無理なのでよく使われるタグの数百個を目安にやっていきます

本来の目的は自作のDanbooruクライアント、Boorutan ( 名称未定 )で使うためのものですが、その他の用途にもお使いいただけます

このリポジトリーは主に四つのsvgファイルと一つのgoファイルで構成されています。
- **danbooru.csv**
  `danbooru.csv`は[webui tag complete](https://github.com/DominikDoom/a1111-sd-webui-tagcomplete)から取ってきたdanbooruのタグが入っています
  danbooru公式が出しているタグリストから取ってこなかった理由は、あちらは1Mタグあり、parseなどが面倒だったためです
- **danbooru-jp.csv**
  `danbooru-jp.csv`は`danbooru.csv`に含まれるタグを手作業で翻訳した物のみが含まれています
- **danbooru-machine-jp.csv**
  `danbooru-machine-jp.csv`は`danbooru.csv`に含まれるタグを手作業で翻訳した`danbooru-jp.csv`を機械翻訳で翻訳した`danbooru-only-machine-jp.csv`で補ったものです。
  よく使われるタグや個人的に気になったタグは手作業で翻訳されていますが、それ以外のタグは機械翻訳です。
- **danbooru-only-machine-jp.csv**
  `danbooru-only-machine-jp.csv`は`danbooru.csv`に含まれるタグを機械翻訳( Google翻訳 )のみで翻訳した物です、正常に翻訳できてないものや誤訳などが大量に含まれます。
  スプレッドシートに以下のような式を入力して翻訳しています
```
=GOOGLETRANSLATE(SUBSTITUTE({tag}, "_", " "),"en","ja")
```
- **main.go**
  `main.go`は翻訳の時に使ってるツールです、タグの翻訳、修正、エクスポートなどができます

# How to use `danbooru-jp.csv`
翻訳したものは`app.db`や`danbooru-jp.csv`に入っています、適当に取り出してください

`danbooru-jp.csv`に関しては[webui tag complete](https://github.com/DominikDoom/a1111-sd-webui-tagcomplete)と互換性があります
```
 ~/stable-diffusiton-webui/extensions/a1111-sd-webui-tagcomplete/tags
```
1. このリポジトリから`danbooru-jp.csv`ファイルをダウンロードする
2. まだ拡張機能を入れてないならwebui tag completeをダウンロードする
3. tagsの中 (上参照) に`danbooru-jp.csv`ファイルを置き
4. webuiを起動し
5. **webui**から**Settings**を開き、サイドバーから**Autocomplete**を選択する
6. 設定項目、**Translation filename**で`danbooru-jp.csv`を選択する ( 無い場合は横の`🔄`ボタンを押す )

これで日本語で`女`と入力すると`1girl`などがサジェストに出るようになりました。

# How to use `main.go`
![main.go](asset/main-go.png)
> 上の画像では`単語を登録する`になっていますが誤記です

`main.go`で単語の翻訳、インポート、エクスポートを行うことができます

エクスポートでは翻訳したものが`danbooru-jp.csv`でエクスポートされ、インポートでは`danbooru.csv`のタグがインポートされます。
まだファイル名などを指定することは出来ません

![main.goで翻訳する画面](asset/main-go-translate.png)
`単語を翻訳する`を選ぶと翻訳画面が出ます、赤く光っている単語が翻訳する単語で、`女の子`と`placeholder`が出ている場所に入力します

`Enter`を押すと入力が確定され次に進みます、出てくる単語はまだ翻訳されておらず、翻訳されていないものの中で上位5個がランダムで出てきます

間違えて入力してしまった場合は`特定の単語を翻訳する`から翻訳してください。
単語を覚えていない場合は
```sql
SELECT name, translated_name, post_count FROM tag WHERE translated = true ORDER BY post_count LIMIT 3;
```
上記のSQLを実行すると直近で翻訳した3件が出てきます

# License
under the `MIT License`