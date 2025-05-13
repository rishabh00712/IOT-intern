const admin = require('firebase-admin');
const serviceAccount = require('./firebaseServiceKey.json');

if (!admin.apps.length) {
  admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
    databaseURL: 'https://your-project-id.firebaseio.com', // ⚠️ Replace with your actual URL
  });
}

module.exports = admin.database(); // Export database instance
