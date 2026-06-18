<?php
include_once('vendor/autoload.php');

$credentials = getenv('SIPGATE_CREDENTIALS');
if(empty($credentials)) {
  die('no credentials defined');
}
$apiUrl = 'https://api.sipgate.com/v2';
// deviceId der sipgate-SIM im SIM800L, deren Absender-Rufnummer gesetzt wird
$deviceId = 'y6';
$code = str_pad(rand(0, 9999), 4, '0', STR_PAD_LEFT);
$callerId = '+491579999' . $code;

// Nur die angezeigte Absender-Rufnummer am Gerät setzen.
// Der eigentliche Anruf (waehlen, 5s warten, auflegen) passiert im Arduino (cache.ino).
makeRequest('/devices/' . $deviceId . '/callerid', 'PUT', array('value' => $callerId));

echo $code;

/* ------------------------ functions ------------------------------------ */
function makeRequest($url, $method, $payload = null) {
  global $credentials, $apiUrl;
  $curl = curl_init();
  $options = array(
    CURLOPT_USERAGENT => 'geocache phone 1.0 PHP',
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_CUSTOMREQUEST => $method,
    CURLOPT_HTTPHEADER => array(
      'Authorization: Basic '.$credentials,
      'Content-Type: application/json',
    ),
    CURLOPT_URL => $apiUrl . $url,
  );
  if(isset($payload)) {
    $options[CURLOPT_POSTFIELDS] = json_encode($payload);
  }
  curl_setopt_array($curl, $options);
  $result = curl_exec($curl);
  if ($result === false) {
    return array('error' => 'getCallStatus Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl));
  }
  curl_close($curl);
  return json_decode($result, true);
}
