#create 256 bit encryption key
openssl rand -base64 32 > ota.key
openssl req -new -nodes -utf8 -sha512 -days 36500 -batch -x509 -config x509.genkey -outform DER -out ota_key.x509 -keyout ota_key.priv
openssl rsa -pubout < ota_key.priv > ota_key.pub
