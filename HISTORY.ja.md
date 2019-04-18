# extzstd の更新履歴

## extzstd-0.3 (平成31年4月)

  * zstd-1.4.0 への更新
  * Zstd::Encoder#write に文字列以外を与えた場合でも文字列に変換するようにして受け入れるように修正
  * Zstd::Decoder#read の buf 引数として nil を与えた場合例外が発生していたため修正
  * zstd に関する例外が発生する時、例外オブジェクトの生成に失敗していたため修正
  * Zstd::EncodeParameter.new に新しいキーワード引数を追加
  * Zstd::Encoder に #close と #pos メソッドを追加
  * `.to_zstd` / `.unzstd` メソッドを `Zstd` リファインメントとして追加
  * `encode` の別名として `compress` を追加
  * `decode` の別名として `decompress` / `uncompress` を追加


## extzstd-0.2 (平成30年2月1日 木曜日)

  * zstd-1.3.3 への更新
  * 細分化されていた例外クラスを Zstd::Error クラスへ集約
  * Zstd::StreamEncoder クラスを Zstd::Encoder クラスへ集約
  * Zstd::StreamDecoder クラスを Zstd::Decoder クラスへ集約


## extzstd-0.1.1 (平成29年3月28日 火曜日)

  * zstd-1.1.4 への更新


## extzstd-0.1 (平成28年11月2日 (水曜日))

初版
