<?php
$credentials = getenv('SIPGATE_CREDENTIALS');
if(empty($credentials)) {
  die('no credentials defined');
}
$apiUrl = 'https://api.sipgate.com/v2';
$code = str_pad(rand(0, 9999), 4, '0', STR_PAD_LEFT);

makeRequest('/devices/y6/callerid', 'PUT', array('value' => '+491579999'.$code));

if($_GET['number']) {
  $sessionId = startCall($_GET['number'], '+491579999' . $code);
  sleep(5);
  hangupCall($sessionId);
}

echo $code;

/* ------------------------ functions ------------------------------------ */
function startCall($caller, $callee) {
  $payload = array(
    'deviceId' => 'y6',
    'caller' => '+' . $caller,
    'callee' => $callee,
    'callerId' => $callee,
  );
  $result = makeRequest('/sessions/calls', 'POST', $payload);
  return $result['sessionId'];
}

function hangupCall($sessionId) {
  return makeRequest('/calls/' . $sessionId, 'DELETE');
}

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