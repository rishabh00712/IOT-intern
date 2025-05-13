const express = require('express');
const multer = require('multer');
const uploadToCloudinary = require('../utils/uploadToCloudinary');
const updateFirebase = require('../utils/updateFirebase');

const router = express.Router();
const upload = multer({ dest: 'uploads/' });

router.post('/upload', upload.single('firmware'), async (req, res) => {
  try {
    const version = req.body.version;
    const filePath = req.file.path;

    const cloudinaryUrl = await uploadToCloudinary(filePath);
    await updateFirebase(version, cloudinaryUrl);

    res.status(200).json({ success: true, message: 'Firmware uploaded and Firebase updated.' });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

module.exports = router;
