#include "spritepropertiesdialog.h"
#include "ui_spritepropertiesdialog.h"

#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QSignalBlocker>

#include "projectcontainer.h"
#include "projecttreemodel.h"
#include "editspritedialog.h"

SpritePropertiesDialog::SpritePropertiesDialog(Sprite &sprite, ProjectTreeModel &projectModel, QWidget *parent) :
    QDialog{parent},
    m_ui{std::make_unique<Ui::SpritePropertiesDialog>()},
    m_sprite{sprite},
    m_projectModel{projectModel}
{
    m_ui->setupUi(this);

    updateTitle();

    if (auto button = m_ui->buttonBox->button(QDialogButtonBox::Ok))
        button->setIcon(QIcon{":/qtgameengine/icons/ok.png"});
    if (auto button = m_ui->buttonBox->button(QDialogButtonBox::Cancel))
        button->setIcon(QIcon{":/qtgameengine/icons/delete.png"});

    m_ui->lineEditName->setText(m_sprite.name);
    updateSpriteInfo();
    m_ui->spinBoxOriginX->setValue(m_sprite.origin.x);
    m_ui->spinBoxOriginY->setValue(m_sprite.origin.y);
    m_ui->checkBoxPreciseCollisionChecking->setChecked(m_sprite.preciseCollisionChecking);
    m_ui->checkBoxSeparateCollisionMasks->setChecked(m_sprite.separateCollisionMasks);
    m_ui->labelPreview->setPixmap(m_sprite.pixmaps.empty() ? QPixmap{} : m_sprite.pixmaps.front());

    connect(&m_projectModel, &ProjectTreeModel::spriteNameChanged,
            this, &SpritePropertiesDialog::spriteNameChanged);

    connect(m_ui->pushButtonLoad, &QAbstractButton::pressed,
            this, &SpritePropertiesDialog::loadSprite);
    connect(m_ui->pushButtonSave, &QAbstractButton::pressed,
            this, &SpritePropertiesDialog::saveSprite);
    connect(m_ui->pushButtonEdit, &QAbstractButton::pressed,
            this, &SpritePropertiesDialog::editSprite);
    connect(m_ui->pushButtonCenterOrigin, &QAbstractButton::pressed,
            this, &SpritePropertiesDialog::centerOrigin);

    connect(m_ui->lineEditName, &QLineEdit::textChanged,
            this, &SpritePropertiesDialog::changed);
    connect(m_ui->spinBoxOriginX, &QSpinBox::valueChanged,
            this, &SpritePropertiesDialog::changed);
    connect(m_ui->spinBoxOriginY, &QSpinBox::valueChanged,
            this, &SpritePropertiesDialog::changed);
    connect(m_ui->checkBoxPreciseCollisionChecking, &QCheckBox::stateChanged,
            this, &SpritePropertiesDialog::changed);
    connect(m_ui->checkBoxSeparateCollisionMasks, &QCheckBox::stateChanged,
            this, &SpritePropertiesDialog::changed);
}

SpritePropertiesDialog::~SpritePropertiesDialog() = default;

void SpritePropertiesDialog::accept()
{
    if (m_sprite.name != m_ui->lineEditName->text())
    {
        if (!m_projectModel.renameSprite(m_sprite, m_ui->lineEditName->text()))
        {
            QMessageBox::critical(this, tr("Renaming Sprite failed!"), tr("Renaming Sprite failed!"));
            return;
        }
    }

    if (m_newPixmaps)
        m_sprite.pixmaps = std::move(*m_newPixmaps);
    m_sprite.origin.x = m_ui->spinBoxOriginX->value();
    m_sprite.origin.y = m_ui->spinBoxOriginY->value();
    m_sprite.preciseCollisionChecking = m_ui->checkBoxPreciseCollisionChecking->isChecked();
    m_sprite.separateCollisionMasks = m_ui->checkBoxSeparateCollisionMasks->isChecked();

    QDialog::accept();
}

void SpritePropertiesDialog::reject()
{
    if (!m_unsavedChanges)
    {
        QDialog::reject();
        return;
    }

    const auto result = QMessageBox::warning(
        this,
        tr("The Sprite has been modified."),
        tr("Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );
    switch (result)
    {
    case QMessageBox::Save:
        accept();
        return;
    case QMessageBox::Discard:
        QDialog::reject();
        return;
    case QMessageBox::Cancel:
        return;
    default:
        qWarning() << "unexpected dialog result" << result;
    }
}

void SpritePropertiesDialog::loadSprite()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Open a Sprite Image..."), {}, tr("BMP Files (*.bmp), PNG Files (*png)"));
    if (path.isEmpty())
        return;

    QPixmap pixmap;
    if (!pixmap.load(path))
    {
        QMessageBox::warning(this, tr("Could not load sprite!"), tr("Could not load sprite!"));
        return;
    }

    m_ui->labelPreview->setPixmap(pixmap);

    m_newPixmaps = std::vector<QPixmap>{ std::move(pixmap) };
    m_unsavedChanges = true;

    updateTitle();
    updateSpriteInfo();
}

void SpritePropertiesDialog::saveSprite()
{
    const auto &pixmaps = m_newPixmaps ? *m_newPixmaps : m_sprite.pixmaps;

    if (pixmaps.empty() || pixmaps.front().isNull())
    {
        QMessageBox::warning(this, tr("No sprites available to save!"), tr("No sprites available to save!"));
        return;
    }

    const auto path = QFileDialog::getSaveFileName(this, tr("Save a Sprite Image..."), m_sprite.name + ".png", tr("PNG Files (*.png)"));
    if (path.isEmpty())
        return;

    if (!pixmaps.front().save(path))
    {
        QMessageBox::warning(this, tr("Could not save Sprite!"), tr("Could not save Sprite!"));
        return;
    }
}

void SpritePropertiesDialog::editSprite()
{
    EditSpriteDialog{m_sprite}.exec();
}

void SpritePropertiesDialog::centerOrigin()
{
    const auto &pixmaps = m_newPixmaps ? *m_newPixmaps : m_sprite.pixmaps;
    if (pixmaps.empty() || pixmaps.front().isNull())
    {
        qDebug() << "unexpected empty pixmaps";
        return;
    }

    m_ui->spinBoxOriginX->setValue(pixmaps.front().width() / 2);
    m_ui->spinBoxOriginY->setValue(pixmaps.front().height() / 2);
}

void SpritePropertiesDialog::changed()
{
    if (!m_unsavedChanges)
    {
        m_unsavedChanges = true;
        updateTitle();
    }
}

void SpritePropertiesDialog::spriteNameChanged(const Sprite &sprite)
{
    if (&sprite != &m_sprite)
        return;

    {
        QSignalBlocker blocker{m_ui->lineEditName};
        m_ui->lineEditName->setText(sprite.name);
    }

    updateTitle();
}

void SpritePropertiesDialog::updateTitle()
{
    setWindowTitle(tr("Sprite Properties: %0%1")
                       .arg(m_sprite.name)
                       .arg(m_unsavedChanges ? tr("*") : QString{})
                   );
}

void SpritePropertiesDialog::updateSpriteInfo()
{
    const auto &pixmaps = m_newPixmaps ? *m_newPixmaps : m_sprite.pixmaps;
    m_ui->labelWidth->setText(tr("Width: %0").arg(pixmaps.empty() ? tr("n/a") : QString::number(pixmaps.front().width())));
    m_ui->labelHeight->setText(tr("Height: %0").arg(pixmaps.empty() ? tr("n/a") : QString::number(pixmaps.front().height())));
    m_ui->labelSubimages->setText(tr("Number of subimages: %0").arg(pixmaps.size()));
}
