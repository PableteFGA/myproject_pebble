var KEY_REQUEST_REFRESH = 1;
var KEY_ITEM_INDEX = 2;
var KEY_ITEM_TEXT = 3;
var KEY_ITEM_COUNT = 4;
var KEY_HEADER_TEXT = 5;
var KEY_ITEM_CODE = 6;
var KEY_ITEM_LABEL = 7;
var KEY_ITEM_RATE_X100 = 8;
var KEY_ITEM_SUPPORTED = 9;

function send(dict, onSuccess) {
  Pebble.sendAppMessage(
    dict,
    function () {
      if (onSuccess) {
        onSuccess();
      }
    },
    function (e) {
      console.log('SEND ERROR -> ' + JSON.stringify(e));
    }
  );
}

function sendTestData() {
  var header = {};
  header[KEY_HEADER_TEXT] = 'Actualizado: UF';
  header[KEY_ITEM_COUNT] = 1;

  send(header, function () {
    var row = {};
    row[KEY_ITEM_INDEX] = 0;
    row[KEY_ITEM_TEXT] = 'UF|39.921,09';
    row[KEY_ITEM_CODE] = 'uf';
    row[KEY_ITEM_LABEL] = 'UF';
    row[KEY_ITEM_RATE_X100] = '3992109';
    row[KEY_ITEM_SUPPORTED] = 1;

    send(row);
  });
}

Pebble.addEventListener('ready', function () {
  console.log('JS listo');
});

Pebble.addEventListener('appmessage', function (e) {
  if (e.payload && e.payload[KEY_REQUEST_REFRESH]) {
    sendTestData();
  }
});