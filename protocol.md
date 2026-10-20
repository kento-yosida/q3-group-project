# Protocol
## Overview
文字列の変換（大文字化・小文字化・長さ確認）および簡易計算（加算・減算・乗算）を行う。
Client は Request を作成し、Server はそれを解析して Response を返す。
## Connection
TCP port: 5000 / UTF-8 / 1行1request / LF終端
1 接続で複数request可。1requestに1responseを返す。
client のEOFで切断。1行はLF込み1023 bytes以下。
上限超過時は接続を閉じる。
## Requests
UPPER <text>
    request: UPPER hello world\n
    response: OK HELLO WORLD\n
    example: UPPER hello\n →OK HELLO\n
    最初の空白の後ろがtext。空白を保存する。
    text は空でないASCII印字可能文字。a-zのみ大文字へ変換。
LOWER
    request: LOWER HELLO WORLD\n
    response: OK hello world\n
    example: LOWER HELLO\n → OK hello\n
    最初の空白の後ろがtext。空白を保存する。
    text は空でないASCII印字可能文字。A-Zのみ大文字へ変換。
ADD
    request: ADD <num1> <num2>\n
    response: OK <result>\nexample: ADD 5 3\n → OK 8\n
    指定された2つの数値を加算する。数値は整数。
SUB
    request: SUB <num1> <num2>\n
    response: OK <result>\n
    example: SUB 10 4\n → OK 6\n
    指定された2つの数値を減算する（num1 - num2）。数値は整数。
MUL
    request: MUL <num1> <num2>\n
    response: OK <result>\n
    example: MUL 3 4\n → OK 12\n指定された2つの数値を乗算する。数値は整数。
## Errors
UNKNOWN\n-> ERROR unknown-command\n
UPPER\n-> ERROR invalid-argument\n
未知commandはunknown-command、引数規則違反はinvalid-argument。
ERROR を返した後も次のrequestを受け付ける。