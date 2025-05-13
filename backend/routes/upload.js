const express = require('express');
const multer = require('multer');
const uploadToCloudinary = require('../utils/uploadToCloudinary');
const updateFirebase = require('../utils/updateFirebase');

const router = express.Router();
const upload = multer({ dest: 'uploads/' });  // Temporary file storage

// Handle firmware upload and notify ESP32 clients
router.post('/upload', upload.single('firmware'), async (req, res) => {
  try {
    const version = req.body.version;

    if (!version || !req.file) {
      return res.status(400).json({
        success: false,
        message: 'Firmware file and version are required.',
      });
    }

    const filePath = req.file.path;

    // Upload to Cloudinary and update Firebase
    const cloudinaryUrl = await uploadToCloudinary(filePath);
    await updateFirebase(version, cloudinaryUrl);

    // Broadcast the firmware URL to ESP32 clients
    const firmwareData = {
      version,
      url: cloudinaryUrl,
    };
    // Notify all connected ESP32 devices about the new firmware
    notifyESP32Clients(firmwareData);

    res.status(200).json({
      success: true,
      message: 'Firmware uploaded and Firebase updated successfully.',
      url: cloudinaryUrl,
    });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

module.exports = (notifyESP32Clients) => router;
