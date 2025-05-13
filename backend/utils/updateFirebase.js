const db = require('../config/firebase');

module.exports = async function updateFirebase(version, url) {
  try {
    await db.ref('firmware').set({ version, url });
  } catch (err) {
    throw new Error('Firebase update failed: ' + err.message);
  }
};
