$(document).ready(function() {
  generateNavHTML();
  bindButtons();
});

function bindButtons() {
  $('.nav').on('click', 'button.about', function() {changeDemo('About/about.html','about');});
  $('.nav').on('click', 'button.3d', function() {changeDemo('DevPage/devPage.html','dev');});
  $('.nav').on('click', 'button.audio', function() {changeDemo('Visualizer/audio.html','audio');});
  $('.nav').on('click', 'button.form', function() {changeDemo('Form/form.html','form');});
  $('.nav').on('click', 'button.soundBoard', function() {changeDemo('SoundBoard/soundBoard.html','soundBoard');});
  $('.nav').on('click', 'button.game', function() {changeDemo('Game/game.html','game');});
}

function changeDemo(url, id) {
  chrome.app.window.create(url, {
    // 'id': id, cant reload window for now
    'bounds': {
      "width" :   window.screen.availWidth,
      "height":   window.screen.availHeight
    }
  }, function (createdWindow) {
    chrome.app.window.current().close();
  })
}

function generateNavHTML() {
  var nav = $('<div>', {'class': 'nav'});
  nav.append($('<button>', {'class': 'about', 'text': 'ABOUT'}));
  nav.append($('<button>', {'class': '3d', 'text': 'SETUP'}));
  nav.append($('<button>', {'class': 'audio', 'text': 'MUSIC'}));
  nav.append($('<button>', {'class': 'form', 'text': 'FORM'}));
  nav.append($('<button>', {'class': 'soundBoard', 'text': 'SOUND BOARD'}));
  nav.append($('<button>', {'class': 'game', 'text': 'GAME'}));
  nav.append($('<div>', {'class': 'connection'}));
  $('body').prepend(nav);
}