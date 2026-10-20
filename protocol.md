Protocol

1. Overview

TCP通信を利用した簡易計算サービス。

clientは計算コマンドと2つの整数をserverへ送信する。serverはコマンドを解析し、計算結果をclientへ返す。

2. Connection

- TCP port: 5000
- 文字コード: UTF-8
- 1行につき1 request
- 行末はLF（"\n"）
- 1 requestに対して1 responseを返す
- 1接続で複数のrequestを送信できる
- clientが切断したら接続を終了する
- 1行の最大長はLFを含めて1023 bytes
- 上限を超えた場合は接続を終了する

3. Request format

コマンドは大文字のみを使用する。

書式は次のとおり。

"COMMAND <整数1> <整数2>\n"

- コマンドと整数は半角スペース1個で区切る
- 行の先頭や末尾に余分な空白を入れない
- 整数は-1000〜1000の範囲
- 整数は10進数で指定する
- 負数には先頭に"-"を付ける
- 正数には"+"を付けない
- 先頭の余分な0は認めない（ただし"0"は有効）

ADD

2つの整数を加算する。

Request:

"ADD 5 3\n"

Response:

"OK 8\n"

SUB

1つ目の整数から2つ目の整数を引く。

Request:

"SUB 10 4\n"

Response:

"OK 6\n"

MUL

2つの整数を乗算する。

Request:

"MUL 3 4\n"

Response:

"OK 12\n"

4. Response format

計算に成功した場合、以下の形式で返す。

"OK <計算結果>\n"

計算結果は符号付き10進整数とし、正数には"+"を付けない。

例：

"ADD -5 3\n" → "OK -2\n"

5. Error response

Unknown command

定義されていないコマンドを受信した場合：

"DIV 10 2\n" → "ERROR unknown-command\n"

小文字のコマンドも未知のコマンドとして扱う。

Invalid argument

引数が不足している、整数ではない、範囲外である、または書式が異なる場合：

"ADD 5\n" → "ERROR invalid-argument\n"

"ADD abc 3\n" → "ERROR invalid-argument\n"

"ADD 2000 3\n" → "ERROR invalid-argument\n"

"ADD 5  3\n" → "ERROR invalid-argument\n"

エラーを返した後も、次のrequestを受け付ける。

6. Implementation status

今回最初に実装する機能：

- ADD（足し算）

今後実装する機能：

- SUB（引き算）
- MUL（掛け算）
- 各種エラー処理

この文書は3種類すべての通信仕様を定義するものであり、現時点ですべての機能が実装済みであることを意味しない。