var weatherReceived = false;
var forecastReceived = false;

function iconFromWeatherId(weatherId) {
  if ((weatherId < 805) && (weatherId > 801))  {
    return 0; // Clouds
  } else if (weatherId == 801) {
    return 1; // Partial cloudy
  } else if (weatherId == 800) {
    return 2; // Sun
  } else if (weatherId == 600) {
    return 3; // Little Snow
  } else if ((weatherId < 617) && (weatherId > 610)) {
    return 4; // Sleet
  } else if ((weatherId < 623) && (weatherId > 599)) {
    return 5; // Snow
  } else if ((weatherId < 532) && (weatherId > 499)) {
    return 6; // Downpour
  } else if ((weatherId < 322) && (weatherId > 299)) {
    return 7; // Rain
  } else if ((weatherId < 233) && (weatherId > 199)) {
    return 8; // T-Storm
  } else {
    return 0; // Clouds
  }
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        if (response && response.sys && response.weather && response.main) {
          var temp = Math.round(response.main.temp - 273.15);
          if (temp > 0) {
            temp = "+" + temp;
          }
          var icon = iconFromWeatherId(response.weather[0].id);
          var city = response.name;
          var pressure = response.main.pressure / 10;
          var temperature = temp + "\u00B0C" + " " + pressure + "kPa";
          var sunrise = response.sys.sunrise;
          var sunset = response.sys.sunset;
          if (!Date.now) {
            Date.now = function() { return new Date().getTime(); };
          }
          var curTime = Date.now() / 1000;
          var isDay = 0;
          if ((curTime > sunrise) && (curTime < sunset)) {
            isDay = 1;
          }
          console.log("sunrise " + sunrise + " sunset " + sunset + " current time " + curTime);
          console.log('temp_now ' + temperature);
          console.log('sky_now ' + icon);
          console.log('city ' + city);
          localStorage.setItem("temp_now", temperature);
          localStorage.setItem("sky_now", icon);
          localStorage.setItem("city", city);
          localStorage.setItem("is_day", isDay);
        }
      } else {
        console.log("unable to fetch weather");
        city = "weather failed";
      }
      weatherReceived = true;
      sendWeather();
    }
  }
  req.send(null);
}

function fetchForecast(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/forecast/daily?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=2&units=metric&mode=json", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        if (response && response.list && response.list.length > 0) {
          var weatherResult = response.list[1];
          var temp_night = Math.round(weatherResult.temp.min);
          if (temp_night > 0) {
            temp_night = '+' + temp_night;
          }
          var temp_day = Math.round(weatherResult.temp.max);
          if (temp_day > 0) {
            temp_day = '+' + temp_day;
          }
          var temperature = temp_night + "..." + temp_day + "\u00B0C";
          var icon = iconFromWeatherId(weatherResult.weather[0].id);
          console.log('temp_tomorrow ' + temperature);
          console.log('sky_tomorrow ' + icon);
          localStorage.setItem("temp_tomorrow", temperature);
          localStorage.setItem("sky_tomorrow", icon);
        }
      } else {
        console.log("Error: unable to fetch forecast");
        city = "forecast failed";
      }
      forecastReceived = true;
      sendWeather();
    }
  }
  req.send(null);
}

var get_weather = 0;
var get_forecast = 0;

function getWeather() {
  window.navigator.geolocation.getCurrentPosition(function(pos) {
  if (get_weather) {
    fetchWeather(pos.coords.latitude, pos.coords.longitude);
  }
  if (get_forecast) {
    fetchForecast(pos.coords.latitude, pos.coords.longitude);
  }
  });
}

function sendWeather() {
  if (weatherReceived && forecastReceived) {
    weatherReceived = false;
    forecastReceived = false;
    var sky_now = localStorage.getItem("sky_now");
    if (!sky_now) {
      sky_now = 0;
    }
    var temp_now = localStorage.getItem("temp_now");
    if (!temp_now) {
      temp_now = "+25" + "\u00B0C";
    }
    var city = localStorage.getItem("city");
    if (!city) {
      city = "connecting";
    }
    var sky_tomorrow = localStorage.getItem("sky_tomorrow");
    if (!sky_tomorrow) {
      sky_tomorrow = 0;
    }
    var temp_tomorrow = localStorage.getItem("temp_tomorrow");
    if (!temp_tomorrow) {
      temp_tomorrow = "+15 ... +25" + "\u00B0C";
    }
    var isDay = localStorage.getItem("is_day");
    if (!isDay) {
      isDay = 1;
    }
    Pebble.sendAppMessage({
        "is_day" : parseInt(isDay),
        "sky_now": parseInt(sky_now),
        "temp_now": temp_now,
        "city": city,
        "sky_tomorrow": parseInt(sky_tomorrow),
        "temp_tomorrow": temp_tomorrow});
  }
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "temperature":"N/A"
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          console.log('message from watch: weather: ' + e.payload.weather + ' forecast: ' + e.payload.forecast); 
                          get_weather = e.payload.weather;
                          get_forecast = e.payload.forecast;
                          getWeather();
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });


