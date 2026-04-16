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

function formatNumber(value) {
  var num = Number(value);
  if (isNaN(num)) {
    return String(value);
  }

  var fixed = num.toFixed(2);
  var parts = fixed.split('.');
  var intPart = parts[0];
  var decPart = parts[1];

  intPart = intPart.replace(/\B(?=(\d{3})+(?!\d))/g, '.');
  return intPart + ',' + decPart;
}

function formatDate(isoDate) {
  if (!isoDate || typeof isoDate !== 'string') {
    return '';
  }

  var parts = isoDate.split('-');
  if (parts.length !== 3) {
    return isoDate;
  }

  return parts[2] + '/' + parts[1] + '/' + parts[0];
}

function buildRows(data) {
  if (!data || !data.uf) {
    return [];
  }

  var api = data.uf;
  var value = Number(api.valor);
  var unit = (api.unidad_medida || '').toString().toLowerCase();

  var supported = !isNaN(value) && value > 0 && unit.indexOf('peso') !== -1;
  var rateX100 = supported ? String(Math.round(value * 100)) : '0';

  return [{
    label: 'UF',
    code: 'uf',
    rowText: 'UF|' + formatNumber(value),
    rateX100: rateX100,
    supported: supported ? 1 : 0
  }];
}

function sendRows(rows, index) {
  if (index >= rows.length) {
    return;
  }

  var row = rows[index];
  var msg = {};
  msg[KEY_ITEM_INDEX] = index;
  msg[KEY_ITEM_TEXT] = row.rowText;
  msg[KEY_ITEM_CODE] = row.code;
  msg[KEY_ITEM_LABEL] = row.label;
  msg[KEY_ITEM_RATE_X100] = row.rateX100;
  msg[KEY_ITEM_SUPPORTED] = row.supported;

  send(msg, function () {
    sendRows(rows, index + 1);
  });
}

function fetchSummary() {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', 'https://findic.cl/api/', true);

  xhr.onload = function () {
    if (xhr.status !== 200) {
      var err = {};
      err[KEY_HEADER_TEXT] = 'Error de red';
      err[KEY_ITEM_COUNT] = 1;
      err[KEY_ITEM_INDEX] = 0;
      err[KEY_ITEM_TEXT] = 'Sin datos';
      err[KEY_ITEM_SUPPORTED] = 0;
      send(err);
      return;
    }

    var data = JSON.parse(xhr.responseText);
    var rows = buildRows(data);

    if (rows.length === 0) {
      var empty = {};
      empty[KEY_HEADER_TEXT] = 'Sin UF';
      empty[KEY_ITEM_COUNT] = 1;
      empty[KEY_ITEM_INDEX] = 0;
      empty[KEY_ITEM_TEXT] = 'Sin datos';
      empty[KEY_ITEM_SUPPORTED] = 0;
      send(empty);
      return;
    }

    var header = {};
    header[KEY_HEADER_TEXT] = 'Actualizado: ' + formatDate(data.fecha);
    header[KEY_ITEM_COUNT] = rows.length;

    send(header, function () {
      sendRows(rows, 0);
    });
  };

  xhr.onerror = function () {
    var err = {};
    err[KEY_HEADER_TEXT] = 'Error de red';
    err[KEY_ITEM_COUNT] = 1;
    err[KEY_ITEM_INDEX] = 0;
    err[KEY_ITEM_TEXT] = 'Sin datos';
    err[KEY_ITEM_SUPPORTED] = 0;
    send(err);
  };

  xhr.send();
}

Pebble.addEventListener('ready', function () {
  console.log('JS listo');
});

Pebble.addEventListener('appmessage', function (e) {
  if (e.payload && e.payload[KEY_REQUEST_REFRESH]) {
    fetchSummary();
  }
});