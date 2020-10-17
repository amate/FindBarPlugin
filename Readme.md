# FindBarPlugin

![](https://raw.githubusercontent.com/amate/FindBarPlugin/images/images/ss1.jpg)

## ■はじめに
このプラグインは、テキスト編集ソフト"[Mery](https://www.haijin-boys.com/wiki/メインページ)"に検索バーを追加するプラグインです

## ■動作環境
・Windows 10 home 64bit バージョン 2004  
Mery (x86) Version 3.1.0
Mery (x64) Version 2.6.7
で動作を確認

## ■導入方法

32bit版の"Mery"なら "FindBarPlugin.dll"を
64bit版の"Mery"なら "FindBarPlugin64.dll"を
それぞれ"Mery.exe"があるフォルダの "Plugins"フォルダ内にコピーしてください

その後、Mery.exeを起動して、メニューから \[表示\]->\[ツールバー\]->\[Find Bar\] とクリックすれば、検索バーが表示されるはずです

同じ"Plugins"フォルダに"SearchToolbar.dll"が存在すると起動に失敗する場合がある？
起動しなかったら"SearchToolbar.dll"を"Plugins"フォルダから抜いてみてください

## ■設定

検索バーの右クリックメニューからさらに詳細な設定が可能です


・"タブバーの空き領域をミドルクリックで、閉じたタブを復元する"
名前の通りの機能です
ついでに、タブバーでの中ボタンダブルクリックも無効化します
復元されるタブは、起動後に閉じたタブに限られます

"新しいウィンドウに移動"で分離したウィンドウのタブを閉じた場合、復元されるタブがおかしくなる既知のバグがあります


・"独自にツールバーの位置を記録/復元する"
Mery Version 3.1.0からなぜか起動時にツールバーの位置が復元されないことがあるので、自前で位置復元を試みます
起動時に検索バーがどっかいっちゃうって人はメニューから有効にしてください


導入後、"Mery.exe"終了時に "Pluings"フォルダに生成される "FindBarSettings.json"を直接編集することで更に以下の設定が可能です

''''
"SearchBar": {
    "SearchEditWidth": 150  /\*←この行を追加\*/
}
''''

検索バーのコンボボックスの幅を指定できます

''''
"SearchBar": {
    "MaxSearchHistory": 64  /\*←この行を追加\*/
}
''''

検索バーの最大履歴数を指定できます

![](https://raw.githubusercontent.com/amate/FindBarPlugin/images/images/ss2.jpg)


## ■その他

このプラグインを導入すると、ついでにMery自前の検索ダイアログで "Shift + Enter"を押すと "前を検索" を実行するようになります
この動作は無効化できませんが、それで困ることはないから大丈夫でしょう

## ■使用ライブラリ

- boost  
https://www.boost.org/

- JSON for Modern C++  
https://github.com/nlohmann/json

- WTL  
http://sourceforge.net/projects/wtl/

## ■ビルド方法
Visual Studio 2019 が必要です  

## ■著作権表示
Copyright (C) 2020 amate

私が書いた部分のソースコードは、MIT License とします。

## ■更新履歴

<pre>

v1.0
・公開

</pre>
