<?php
/*
Copyright (C) 2019  Justus Croskery
To contact me, email me at justus@olmmcc.tk.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
*/
include_once '/etc/httpd/privateVars.php';
define('apiKey', $key);
define('email', 'justus.croskery@gmail.com');

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, "https://ipecho.net/plain");
curl_setopt($ch, CURLOPT_HEADER, 0);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
define('ip', curl_exec($ch));
curl_close($ch);

function makeCurlRequest($baseURL, $httpArray = [], $put = false)
{
    $params = '';
    if (!empty($httpArray) && !$put) {
        $params = http_build_query($httpArray);
    }
    $curl = curl_init();
    curl_setopt($curl, CURLOPT_URL, $baseURL . $params);
	curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
	if($put){
		curl_setopt($curl, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_setopt($curl, CURLOPT_POSTFIELDS,json_encode($httpArray));
	}
    curl_setopt($curl, CURLOPT_HTTPHEADER, array(
        'X-Auth-Email: ' . email,
        'X-Auth-Key: ' . apiKey,
        'Content-Type: application/json',
    ));
    $response = json_decode(curl_exec($curl), true);
    curl_close($curl);
    return $response;
}
function changeIp($name)
{
    $response = makeCurlRequest('https://api.cloudflare.com/client/v4/zones/', []);
    $zoneid = $response['result'][0]['id'];

    $arrayPassed = [
        'type' => 'A',
        'name' => $name
    ];
    $response = makeCurlRequest('https://api.cloudflare.com/client/v4/zones/' . $zoneid . '/dns_records?', $arrayPassed);
    $recordid = $response['result'][0]['id'];

    $response = makeCurlRequest('https://api.cloudflare.com/client/v4/zones/' . $zoneid . '/dns_records/' . $recordid);
    $currentIp = $response['result']['content'];
    if (ip == $currentIp) {
        echo $response['result']['name'] . ' has already been updated.';
    } else {
		echo $response['result']['name'] . ' must be updated.';
		$arrayPassed = [
			'type' => 'A',
			'name' => $name,
			'content' => ip,
			'proxied' => false
		];
		$response = makeCurlRequest('https://api.cloudflare.com/client/v4/zones/' . $zoneid . '/dns_records/' . $recordid, $arrayPassed, true);
		print_r($response);
    }
}
$domains = ['olmmcc.tk', 'www.olmmcc.tk'];
foreach($domains as $domain){
	changeIp($domain);
}
