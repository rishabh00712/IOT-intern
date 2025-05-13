const cloudinary = require('cloudinary').v2;
const fs = require('fs');

cloudinary.config({
  cloud_name: 'YOUR_CLOUD_NAME',
  api_key: 'YOUR_API_KEY',
  api_secret: 'YOUR_API_SECRET',
});

module.exports = function uploadToCloudinary(filePath) {
  return new Promise((resolve, reject) => {
    cloudinary.uploader.upload(
      filePath,
      { resource_type: 'raw' }, // for .bin files
      (error, result) => {
        fs.unlinkSync(filePath); // Clean up local file
        if (error) return reject(error);
        resolve(result.secure_url);
      }
    );
  });
};
