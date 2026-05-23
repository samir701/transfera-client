/** Max upload size enforced in the UI (must match product policy). */
export const MAX_UPLOAD_MB = 5;
export const MAX_UPLOAD_BYTES = MAX_UPLOAD_MB * 1024 * 1024;

const VIDEO_MIME_PREFIX = 'video/';

const VIDEO_EXTENSIONS = new Set([
  '.3g2',
  '.3gp',
  '.asf',
  '.avi',
  '.f4v',
  '.flv',
  '.m2ts',
  '.m4v',
  '.mkv',
  '.mov',
  '.mp4',
  '.mpeg',
  '.mpg',
  '.mts',
  '.ogv',
  '.ts',
  '.webm',
  '.wmv',
]);

function fileExtension(name: string): string {
  const dot = name.lastIndexOf('.');
  if (dot <= 0 || dot === name.length - 1) return '';
  return name.slice(dot).toLowerCase();
}

export function isVideoFile(file: File): boolean {
  if (file.type && file.type.toLowerCase().startsWith(VIDEO_MIME_PREFIX)) {
    return true;
  }
  return VIDEO_EXTENSIONS.has(fileExtension(file.name));
}

export function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < MAX_UPLOAD_BYTES) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

/** Returns a user-facing error message, or null if the file is allowed. */
export function validateUploadFile(file: File): string | null {
  if (file.size > MAX_UPLOAD_BYTES) {
    return `File must be ${MAX_UPLOAD_MB} MB or smaller (selected: ${formatFileSize(file.size)}).`;
  }
  if (isVideoFile(file)) {
    return 'Video files are not allowed. Choose an image, document, or other non-video file.';
  }
  return null;
}
