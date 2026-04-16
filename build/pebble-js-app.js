/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId])
/******/ 			return installedModules[moduleId].exports;
/******/
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			exports: {},
/******/ 			id: moduleId,
/******/ 			loaded: false
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.loaded = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

	__webpack_require__(1);
	module.exports = __webpack_require__(2);


/***/ }),
/* 1 */
/***/ (function(module, exports) {

	/**
	 * Copyright 2024 Google LLC
	 *
	 * Licensed under the Apache License, Version 2.0 (the "License");
	 * you may not use this file except in compliance with the License.
	 * You may obtain a copy of the License at
	 *
	 *     http://www.apache.org/licenses/LICENSE-2.0
	 *
	 * Unless required by applicable law or agreed to in writing, software
	 * distributed under the License is distributed on an "AS IS" BASIS,
	 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	 * See the License for the specific language governing permissions and
	 * limitations under the License.
	 */
	
	(function(p) {
	  if (!p === undefined) {
	    console.error('Pebble object not found!?');
	    return;
	  }
	
	  // Aliases:
	  p.on = p.addEventListener;
	  p.off = p.removeEventListener;
	
	  // For Android (WebView-based) pkjs, print stacktrace for uncaught errors:
	  if (typeof window !== 'undefined' && window.addEventListener) {
	    window.addEventListener('error', function(event) {
	      if (event.error && event.error.stack) {
	        console.error('' + event.error + '\n' + event.error.stack);
	      }
	    });
	  }
	
	})(Pebble);


/***/ }),
/* 2 */
/***/ (function(module, exports) {

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

/***/ })
/******/ ]);
//# sourceMappingURL=pebble-js-app.js.map