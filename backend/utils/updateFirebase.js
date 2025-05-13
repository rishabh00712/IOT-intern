const axios = require('axios');

const FIREBASE_URL = 'https://your-project-id.firebaseio.com/firmware.json';

module.exports = async function updateFirebase(version, url) {
  const payload = {
    version: version,
    url: url,
  };

  await axios.put(FIREBASE_URL, payload);
};
