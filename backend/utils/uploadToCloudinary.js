const cloudinary = require('cloudinary').v2;
const fs = require('fs');

// Setup cloudinary config (you should use dotenv in production)
cloudinary.config({
  cloud_name: process.env.CLOUDINARY_CLOUD_NAME,
  api_key: process.env.CLOUDINARY_API_KEY,
  api_secret: process.env.CLOUDINARY_API_SECRET,
});

module.exports = async function uploadToCloudinary(filePath) {
  try {
    const result = await cloudinary.uploader.upload(filePath, {
      resource_type: 'raw', // Needed for .bin files
    });

    fs.unlink(filePath, () => {}); // Delete local file after upload
    return result.secure_url;

  } catch (err) {
    fs.unlink(filePath, () => {}); // Cleanup on error
    throw new Error('Cloudinary upload failed: ' + err.message);
  }
};
